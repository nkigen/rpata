#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#define HELLO_PORT 12345
#define HELLO_GROUP "225.0.0.37"
#define MSGBUFSIZE 256

static bool mcast_svr(struct sockaddr_in *addr, int *fd, char *from)
{
	if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return false;
	}

	memset(addr, 0, sizeof *addr);
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(HELLO_GROUP);
	addr->sin_port = htons(HELLO_PORT);

	int i = 0;
	while (1) {
		char message[128];
		sprintf(message, "((%s)) Hello, World!. #SEQ: %d", from, ++i);
		if (sendto(*fd, message, strlen(message), 0, (struct sockaddr *)addr, sizeof *addr) < 0) {
			perror("sendto");
			return false;
		}
		printf("[SEND] %s\n", message);
		sleep(1);
	}

	return true;
}

static bool mcast_cli(struct sockaddr_in *addr, int *fd)
{
	int nbytes,addrlen;
	struct ip_mreq mreq;
	char msgbuf[MSGBUFSIZE];

	if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return false;
	}

	int yes = 1;
	if (setsockopt(*fd,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
		perror("Reusing ADDR failed");
		return false;
	}

	memset(addr, 0, sizeof *addr);
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(HELLO_PORT);

	if (bind(*fd, (struct sockaddr *)addr, sizeof *addr) < 0) {
		perror("bind");
		return false;
	}

	mreq.imr_multiaddr.s_addr = inet_addr(HELLO_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(*fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof mreq) < 0) {
		perror("setsockopt");
		return false;
	}

	while (1) {
		addrlen = sizeof *addr;
		if ((nbytes = recvfrom(*fd, msgbuf, MSGBUFSIZE, 0, (struct sockaddr *) addr, &addrlen)) < 0) {
			perror("recvfrom");
			return false;
		}
		printf("[RECV]");// %s || len %d\n", msgbuf, nbytes);
		int j = 0;
		while(j < nbytes){
			printf("%c", msgbuf[j]);
			j++;
		}
		printf("\n");
	}
	return true;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	int fd;

#if defined SVR
	mcast_svr(&addr, &fd, argv[1]);
#elif defined CLI
	mcast_cli(&addr, &fd);
#endif
	return 0;
}
