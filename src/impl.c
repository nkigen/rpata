#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <sys/sysinfo.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <rpata.h>
#include "impl.h"

static bool ipaddr_init(struct rpata_ipaddr **ip)
{
	FILE *fp = popen("ls -A /sys/class/net", "r");
	if(!fp)
		return false;

	char out[10];
	(*ip)->nr_ips = 0;
	while(fgets(out, 9, fp))
		++(*ip)->nr_ips;

	if(!(*ip)->nr_ips)
		return false;

	(*ip)->ips = calloc((*ip)->nr_ips, sizeof *(*ip)->ips);
	if(!(*ip)->ips)
		return false;

	memset((*ip)->ips, 0, (*ip)->nr_ips * sizeof *(*ip)->ips);
	return true;
}

static bool iface_init(struct rpata *ctx)
{
	ctx->ips = malloc(sizeof *ctx->ips);
	if(!ctx->ips)
		return false;

	if(!ipaddr_init(&ctx->ips))
		return false;

	struct ifaddrs *ifaddr = NULL;
	struct ifaddrs *ifa = NULL;
	char host[NI_MAXHOST];
	int s = 0;
	int i = 0;

	if(-1 == getifaddrs(&ifaddr))
		return false;

	for(ifa = ifaddr, i = 0; ifa != NULL; ifa = ifa->ifa_next) {
		if(NULL == ifa->ifa_addr){
			continue;
		}

		s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (ifa->ifa_addr->sa_family == AF_INET) {
			if (0 != s) {
				freeifaddrs(ifaddr);
				return false;
			}

			ctx->ips->ips[i].name = strdup(ifa->ifa_name);
			ctx->ips->ips[i].addr = strdup(host);
			++i;
		}
	}

	freeifaddrs(ifaddr);
	return true; 
}	

static void guid_init(uuid_t *guid)
{
	uuid_generate_time_safe(*guid);
}

static void timespec_add_ms(struct timespec *t, uint16_t ms)
{
	t->tv_sec += ms / 1000;
	t->tv_nsec += (ms % 1000) * 1000000;
	if (t->tv_nsec > 1000000000) {
		t->tv_nsec -= 1000000000;
		t->tv_sec += 1;
	}
}

static bool send_init(struct rpata *ctx)
{
	if ((ctx->send_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return false;
	}

	memset(&ctx->send_addr, 0, sizeof ctx->send_addr);
	ctx->send_addr.sin_family = AF_INET;
	ctx->send_addr.sin_addr.s_addr = inet_addr(ctx->mcast);
	ctx->send_addr.sin_port = htons(ctx->mcast_port);

	return true;
}

static bool recv_init(struct rpata *ctx)
{
	struct ip_mreq mreq;

	if ((ctx->recv_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return false;
	}

	int yes = 1;
	if (setsockopt(ctx->recv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		perror("Reusing ADDR failed");
		return false;
	}

	memset(&ctx->recv_addr, 0, sizeof ctx->recv_addr);
	ctx->recv_addr.sin_family = AF_INET;
	ctx->recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ctx->recv_addr.sin_port = htons(ctx->mcast_port);

	if (bind(ctx->recv_fd, (struct sockaddr *)&ctx->recv_addr, sizeof ctx->recv_addr) < 0) {
		perror("bind");
		return false;
	}

	mreq.imr_multiaddr.s_addr = inet_addr(ctx->mcast);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(ctx->recv_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof mreq) < 0) {
		perror("setsockopt");
		return false;
	}

	return true;
}

static void send_mcast(struct rpata *ctx, struct rpata_msg *msg)
{
	if (sendto(ctx->send_fd, msg, sizeof *msg, 0, (struct sockaddr *)&ctx->send_addr, sizeof ctx->send_addr) < 0) {
		perror("sendto");
	}
}

static void recv_mcast(struct rpata *ctx, struct rpata_msg *msg)
{
	int nbytes;
	socklen_t addrlen = sizeof ctx->recv_addr;

	if ((nbytes = recvfrom(ctx->recv_fd, msg, sizeof *msg, 0, (struct sockaddr *)&ctx->recv_addr, &addrlen)) < 0) {
		perror("recvfrom");
	}
}

static bool add_peer(struct rpata *ctx, char *uuid, char *ipaddr)
{
	struct rpata_peer *head = ctx->peers;
	
	struct rpata_peer *peer = malloc(sizeof *peer);
	if(!peer)
		return false;

	uuid_parse(uuid, peer->guid);
	peer->ipaddr = ipaddr;
	peer->next = NULL;

	if(!head){
		ctx->peers = peer;
		goto ret_true;
	}

	while(head->next)
		head = head->next;
	head->next = peer;

ret_true:
	if(ctx->cbacks && ctx->cbacks->peer_joined)
		ctx->cbacks->peer_joined(ipaddr);

	return true;
}

static bool is_peer_new(struct rpata *ctx, uuid_t uuid)
{
	struct rpata_peer *head = ctx->peers;
	while(head){
		if(0 == uuid_compare(uuid, head->guid))
			return false;
		head = head->next;
	}

	return true;
}

static void process_send(struct rpata *ctx)
{
	static int seq = 0;
	struct rpata_msg msg;
	msg.magic = seq++;
	char uuid_str[37];
        uuid_unparse_lower(ctx->guid, uuid_str);
	strcpy(msg.guid, uuid_str);
	strcpy(msg.ip, ctx->ips->ips[0].addr);
	send_mcast(ctx, &msg);
/*	printf("[send] %s, seq %d\n", uuid_str, msg.magic); */
}

static void process_recv(struct rpata *ctx)
{
	struct rpata_msg *msg = malloc(sizeof *msg);
	recv_mcast(ctx, msg);
	uuid_t uuid;
	uuid_parse(msg->guid, uuid);
	while(!uuid_compare(uuid, ctx->guid)){
		recv_mcast(ctx, msg);
		uuid_parse(msg->guid, uuid);
	}

	if(is_peer_new(ctx, uuid)){
		pthread_mutex_lock(&ctx->mutex);
		add_peer(ctx, msg->guid, msg->ip);
		pthread_mutex_unlock(&ctx->mutex);
	}

/*	printf("[recv] %s, seq=%d\n", msg->guid, msg->magic); */
	free(msg);
}

static void *periodic(void *data)
{
	struct rpata *ctx = data;

	send_init(ctx);
	recv_init(ctx);

	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	timespec_add_ms(&t, 1000);
	while(1) {
		process_send(ctx);
		process_recv(ctx);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		timespec_add_ms(&t, 1000);
	}

	return NULL;
}

static void rpata_defaults(struct rpata *ctx)
{
	ctx->mcast = NULL;
	ctx->peers = NULL;
	ctx->nr_peers = 0;
	ctx->mcast_port = RPATA_MCAST_PORT;
	ctx->start = false;

	ctx->cbacks = NULL;
	pthread_mutex_init(&ctx->mutex, NULL);
}

struct rpata *rpata_init()
{
	struct rpata *ctx = malloc(sizeof *ctx);
	if(!ctx)
		return NULL;
	
	rpata_defaults(ctx);
	iface_init(ctx);
	guid_init(&ctx->guid);	

	return ctx;
}

static bool ifacecmp(const char *i1, const char *i2)
{
	return 0 == strcmp(i1, i2);
}

static bool add_iface(struct rpata *ctx, char *iface)
{
	struct rpata_ipaddr *ips = ctx->ips;
	if(!ips)
		return false;

	bool ret = false;
	int i;
	for(i = 0; i < ips->nr_ips; ++i){
		if(ips->ips[i].addr && 
				ifacecmp(iface, ips->ips[i].name)){
			ret = true;
			break;
		}
	}

	return ret;
}

static bool set_mcastaddr(struct rpata *ctx, char *addr)
{
	if(ctx->mcast)
		free(ctx->mcast);

	ctx->mcast = strdup(addr);

	return true;
}

static bool set_mcastport(struct rpata *ctx, char *port)
{
	ctx->mcast_port = atoi(port);
	return true;
}

bool rpata_setopt(struct rpata *ctx, int opt, char *val)
{
	if(ctx->start)
		return false;

	bool ret = false;
	switch(opt){
		case USE_IFACE:
			ret = add_iface(ctx, val);
			break;
		case USE_MCAST_ADDR:
			ret = set_mcastaddr(ctx, val);
			break;
		case USE_MCAST_PORT:
			ret = set_mcastport(ctx, val);
			break;
		default:
			ret = false;
	};

	return ret;
}

void rpata_setcallback(struct rpata *ctx, struct rpata_callback *cback)
{
	ctx->cbacks = cback;
}

bool rpata_start(struct rpata *ctx)
{
	if(pthread_create(&ctx->thread, NULL, periodic, ctx)){
		return false;
	}

	ctx->start = true;
	return true;
}

static void destroy_ips(struct rpata_ipaddr *ips)
{
	for(int i = 0; i < ips->nr_ips; ++i) {
		if(ips->ips[i].addr)
			free(ips->ips[i].addr);
		if(ips->ips[i].name)
			free(ips->ips[i].name);
	}
	free(ips->ips);
	free(ips);
}
static void destroy_peers(struct rpata_peer *peers)
{
	struct rpata_peer *tmp;
	while(peers){
		tmp = peers->next;
		free(peers->ipaddr);
		free(peers);
		peers = tmp;
	}
}

void rpata_destroy(struct rpata *ctx)
{
	destroy_peers(ctx->peers);
	destroy_ips(ctx->ips);
	free(ctx->mcast);
	pthread_mutex_destroy(&ctx->mutex);
}
