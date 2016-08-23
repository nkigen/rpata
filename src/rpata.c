#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <rpata.h>

#include "utils.h"
#include "impl.h"

static bool ipaddr_init(struct rpata_ipaddr **ip)
{
	struct ifaddrs *addrs,*tmp;
	getifaddrs(&addrs);
	for(tmp = addrs, (*ip)->nr_ips = 0; tmp ; tmp = tmp->ifa_next)
	{
		if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
			  ++(*ip)->nr_ips;
	}

	freeifaddrs(addrs);

	if(!(*ip)->nr_ips)
		return false;

	(*ip)->ips = calloc((*ip)->nr_ips, sizeof *(*ip)->ips);
	if(!(*ip)->ips)
		return false;

	memset((*ip)->ips, 0, (*ip)->nr_ips * sizeof *(*ip)->ips);
	return true;
}

static bool ni_init(struct rpata *ctx)
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

	struct sockaddr_in recv_addr;
	memset(&recv_addr, 0, sizeof recv_addr);
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	recv_addr.sin_port = htons(ctx->mcast_port);

	if (bind(ctx->recv_fd, (struct sockaddr *)&recv_addr, sizeof recv_addr) < 0) {
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

static void recv_mcast(struct rpata *ctx, struct rpata_msg *msg, char *recvip)
{
	int nbytes;
	struct sockaddr_in recv_addr;
	socklen_t len = sizeof recv_addr;
	if ((nbytes = recvfrom(ctx->recv_fd, msg, sizeof *msg, 0, (struct sockaddr *)&recv_addr, &len)) < 0) {
		perror("recvfrom");
		return;
	}
	getnameinfo((struct sockaddr *)&recv_addr, sizeof(struct sockaddr_in), recvip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
}

static bool add_peer(struct rpata *ctx, char *uuid, char *ipaddr)
{
	struct rpata_peer *head = ctx->peers;
	
	struct rpata_peer *peer = malloc(sizeof *peer);
	if(!peer)
		return false;

	uuid_parse(uuid, peer->guid);
	peer->ipaddr = strdup(ipaddr);
	clock_gettime(CLOCK_MONOTONIC, &peer->lmsg);
	peer->state = RPATA_PEER_ALIVE;
	peer->next = NULL;

	if(!head){
		ctx->peers = peer;
		goto ret_true;
	}

	while(head->next)
		head = head->next;
	head->next = peer;

ret_true:
	++ctx->nr_peers;
	if(ctx->cbacks && ctx->cbacks->peer_joined)
		ctx->cbacks->peer_joined(ipaddr);

	return true;
}

static bool is_peer_new(struct rpata *ctx, uuid_t uuid)
{
	struct rpata_peer *head = ctx->peers;
	while(head){
		if(0 == uuid_compare(uuid, head->guid)){
			/**FIXME: Move updating time to another function*/
			clock_gettime(CLOCK_MONOTONIC, &head->lmsg);
			head->state = RPATA_PEER_ALIVE;
			return false;
		}
		head = head->next;
	}

	return true;
}

void rpata_peer_getipaddr(struct rpata_peer *peer, char *ip, int pos)
{
	strcpy(ip, peer[pos].ipaddr);
}

void rpata_getpeers(struct rpata *ctx, struct rpata_peer **peers, int *num)
{
	*peers = calloc(ctx->nr_peers, sizeof **peers);
	if(!*peers)
		return;

	*num = 0;
	memset(*peers, 0, ctx->nr_peers * sizeof **peers);
	struct rpata_peer *head = ctx->peers;
	for(int i = 0; i < ctx->nr_peers; ++i, head = head->next){
		if(RPATA_PEER_ALIVE == head->state){
			(*peers)[i].ipaddr = strdup(head->ipaddr);
			++(*num);
			/**FIXME: Add more fields */
		}
	}
}

static void process_send(struct rpata *ctx)
{
	struct rpata_msg msg;
	msg.magic = rpata_magic;
	char uuid_str[37];
        uuid_unparse_lower(ctx->guid, uuid_str);
	strcpy(msg.guid, uuid_str);
	send_mcast(ctx, &msg);
}

static void process_recv(struct rpata *ctx)
{
	struct rpata_msg *msg = malloc(sizeof *msg);
	char recvip[16];
	recv_mcast(ctx, msg, recvip);
	uuid_t uuid;
	uuid_parse(msg->guid, uuid);
	while(!uuid_compare(uuid, ctx->guid)){
		recv_mcast(ctx, msg, recvip);
		uuid_parse(msg->guid, uuid);

		if(is_peer_new(ctx, uuid)){
			pthread_mutex_lock(&ctx->mutex);
			add_peer(ctx, msg->guid, recvip);
			pthread_mutex_unlock(&ctx->mutex);
		}
	}


	free(msg);
}

static void *periodic(void *data)
{
	struct rpata *ctx = data;

	send_init(ctx);
	recv_init(ctx);

	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	rpata_timespec_add_ms(&t, ctx->period);
	while(1) {
		process_send(ctx);
		process_recv(ctx);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		rpata_timespec_add_ms(&t, ctx->period);
	}

	return NULL;
}

static void rpata_defaults(struct rpata *ctx)
{
	ctx->mcast = NULL;
	ctx->peers = NULL;
	ctx->nr_peers = 0;
	ctx->period = RPATA_PERIOD;
	ctx->timeout = RPATA_TIMEOUT;
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
	ni_init(ctx);
	guid_init(&ctx->guid);	

	return ctx;
}

int rpata_getsize(struct rpata *ctx)
{
	int size;
	pthread_mutex_lock(&ctx->mutex);
	size = ctx->nr_peers;
	pthread_mutex_unlock(&ctx->mutex);

	return size;
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
	for(size_t i = 0; i < ips->nr_ips; ++i) {
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
