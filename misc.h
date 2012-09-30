#ifndef _MISC_H_
#define _MISC_H_

#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <utlist.h>

/* 
 * tuple is operated by utlist.
 */


/* prefix tuple */

struct prefix {
	int mask_len;
	struct sockaddr_storage addr;
};

struct prefix_tuple {

	struct prefix prefix;
	struct prefix_tuple * next;
	struct prefix_tuple * prev;
};

int cmp_prefix_tuple (struct prefix_tuple * p1, struct prefix_tuple * p2);



/* address tuple */
struct addr_tuple {
	struct sockaddr_storage addr;

	struct addr_tuple * next;
	struct addr_tuple * prev;
};

int cmp_addr_tuple (struct addr_tuple * a1, struct addr_tuple * a2);



/* LOCAL EID TUPLE */
struct locator {
	struct sockaddr_storage loc_addr;
	u_int8_t priority, weight, m_priority, m_weight;
};

struct eid_tuple {
	char	iface[IFNAMSIZ];
	char 	authkey[LISP_MAX_KEYLEN];
	struct prefix	eid_prefix;
	struct locator	locator;

	struct eid_tuple * next;
	struct eid_tuple * prev;
};	



#endif 
