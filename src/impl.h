#ifndef RPATA_IMPL_H
#define RPATA_IMPL_H
#include <pthread.h>
#include <uuid/uuid.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <rpata.h>

static const int const rpata_magic = 111;

/*
 * Defaults
 */
enum{
	RPATA_MCAST_PORT = 17000,
	RPATA_PERIOD = 1000,
	RPATA_TIMEOUT = 5000,
};

struct rpata_msg{
	uint8_t magic;
	char guid[37];
};

struct rpata_ipaddr{
	size_t nr_ips;		/**< Number of IP addresses */
	struct _ip{
		char *addr;	/**<IP address */
		char *name;	/**< NIC name */
	} *ips;
};

enum rpata_peer_state{
	RPATA_PEER_ALIVE,
	RPATA_PEER_AWOL,
};

struct rpata_peer{
	uuid_t guid;
	char *ipaddr;
	enum rpata_peer_state state;
	struct timespec lmsg;
	struct rpata_peer *next;
};

/**
 * RPata context
 */
struct rpata{
	uuid_t guid;			/**< Globally unique identifier */
	struct rpata_ipaddr *ips;	/**< IP addresses associated to the node */
	char *mcast;			/**< Multicast address to use */
	char *ni;			/**< Network interface to use */

	bool start;			/**< Service already started */
	uint16_t mcast_port;		/**< Multicast Port */
	int send_fd;
	int recv_fd;

	struct sockaddr_in send_addr;

	struct rpata_callback *cbacks;

	int period;
	int timeout;

	int nr_peers;
	struct rpata_peer *peers;

	pthread_mutex_t mutex;
	pthread_t thread;		/**< Thread */
};

#endif /* RPATA_IMPL_H */
