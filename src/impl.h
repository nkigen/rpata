#ifndef RPATA_IMPL_H
#define RPATA_IMPL_H

#include <stdint.h>
#include <pthread.h>

#include <rpata.h>

struct ipaddr{
	size_t nlen;		/**< Number of IP addresses */
	struct _ip{
		char *addr;	/**<IP address */
		char name[16];	/**< NIC name */
	} *ips;
};

/**
 * RPata context
 */
struct rpata{
	char guid[128];		/**< Globally unique identifier */
	struct ipaddr *ips;	/**< IP addresses associated to the node */

	pthread_mutex_t mutex;	/**< Mutex lock */

	pthread_t thread;	/**< Thread */
};

struct rpata *rpata_init();
#endif /* RPATA_IMPL_H */
