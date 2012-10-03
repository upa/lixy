#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <pthread.h>
#include <utlist.h>

#include "lisp.h"
#include "list/list.h"
#include "patricia/patricia.h"


#define LISP_MAP_REGIST_INTERVAL	10


struct locator {
	struct sockaddr_storage loc_addr;
	u_int8_t priority, weight, m_priority, m_weight;
};

struct eid {			/* EID Instance */
	pthread_t tid;		/* Pthread ID of this EID instance*/
	int raw_socket;		/* Raw socket for own interface */

	char iface[IFNAMSIZ];
	char authkey[LISP_MAX_KEYLEN];
	prefix_t prefix;
	struct locator locator;
};


struct lisp {
	int udp_socket;	/* socket for sending encapsulated LISP packet		*/
	
	list_t * eid_tuple;			/* Local EID List		*/
	struct locator loc;			/* Local Locator Address 	*/
	struct sockaddr_storage mapsrvaddr;	/* Map Server Address		*/
};

extern struct lisp lisp;


#endif /* _COMMON_H_ */
