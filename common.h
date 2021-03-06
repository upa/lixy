#ifndef _COMMON_H_
#define _COMMON_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <pthread.h>
#include <utlist.h>
#include <net/ethernet.h>

#include "lisp.h"
#include "list/list.h"
#include "patricia/patricia.h"

#define LISP_TUNNAME			"lisp0"
#define LISP_MAP_REGISTER_INTERVAL	10
#define LISP_EID_NAME_LEN		16

#define LIXY_LOGFACILITY		LOG_LOCAL7
#define LIXY_LOGNAME			"lixy"

#define ADDRBUFLEN 56

/* DO NOT CHANGE ORDER OF MEMBERS!! */
struct locator {
	struct sockaddr_storage loc_addr;
	u_int8_t priority, weight, m_priority, m_weight;
};

struct eid {			/* EID Instance */
	char name[LISP_EID_NAME_LEN];
	char authkey[LISP_MAX_KEYLEN];
	list_t * prefix_tuple;
};


#include "maptable.h"

struct lisp {
	int udp_socket;		/* socket for sending encapsulated packet */
	int ctl_socket; 	/* socket for sending control LISP packet */
	int cmd_socket;		/* socket for configuration command */
	int raw_socket; 	/* socket for dump packets */
	int tun_socket;		/* socket for lisp0 interface */

	list_t * eid_tuple;	/* Local EID List */
	list_t * loc_tuple;	/* locator address list	*/
	list_t * mapsrv_tuple;	/* Map Server List */

	struct maptable * rib;	/* Map Table		*/

	list_t * cmd_tuple;	/* configuration command list */
	char ** ctl_message;	/* return message to control */

	pthread_t map_register_t;
	pthread_t map_message_t;
	pthread_t maptable_t;
	pthread_t lisp_dp_t;
	pthread_t lisp_dp_tun_t;
	pthread_t lisp_raw_packet_t;
	pthread_t lisp_op_t;
};

extern struct lisp lisp;

#define LISP_UNIX_DOMAIN	"/var/run/lixy"
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 104
#endif


#endif /* _COMMON_H_ */


