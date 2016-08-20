#ifndef RPATA_IMPL_H
#define RPATA_IMPL_H
#include <pthread.h>
#include <uuid/uuid.h>

#include <rpata.h>

#define MCAST_PORT	17000

struct rpata_msg{
	uint8_t magic;
	char guid[37];
	char ip[16];
};

struct rpata_ipaddr{
	size_t nr_ips;		/**< Number of IP addresses */
	struct _ip{
		char *addr;	/**<IP address */
		char *name;	/**< NIC name */
	} *ips;
};

struct rpata_peer{
	uuid_t guid;
	char *ipaddr;
	struct rpata_peer *next;
};

/**
 * RPata context
 */
struct rpata{
	uuid_t guid;			/**< Globally unique identifier */
	struct rpata_ipaddr *ips;	/**< IP addresses associated to the node */
	char *mcast;			/**< Multicast address to use */

	int send_fd;
	int recv_fd;

	struct sockaddr_in send_addr;
	struct sockaddr_in recv_addr;

	int nr_peers;
	struct rpata_peer *peers;

	pthread_t thread;		/**< Thread */
};

#endif /* RPATA_IMPL_H */
