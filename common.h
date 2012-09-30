#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/epoll.h>

#include "misc.h"

#define LISP_MAP_REGIST_INTERVAL	10



struct lisp {
	int rawskepfd;	/* epoll fd listening for raw sockets connected to EID	*/
	int udp_socket;	/* socket for sendign encapsulated LISP packet		*/
	int raw_socket;	/* socket for sending decapsulated packet to EID	*/
	
	struct eid_tuple 	* eid_tuple;		/* Local EID LIST	*/
	struct addr_tuple	* loc_tuple;		/* Locator Address	*/
	struct sockaddr_storage mapsrvaddr;
};

extern struct lisp lisp;

#endif /* _COMMON_H_ */
