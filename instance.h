#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#include <pthread.h>
#include "common.h"

#define LISP_EID_DP_BUF_LEN	2048

/* EID Instance */
struct eid * create_eid_instance (char * eidname);
struct eid * delete_eid_instance (struct eid * eid);

int set_eid_iface (struct eid * eid, char * ifname);
int set_eid_authkey (struct eid * eid, char * authkey);
int set_eid_prefix (struct eid * eid, prefix_t * prefix);

int unset_eid_iface (struct eid * eid);
int unset_eid_authkey (struct eid * eid);
int unset_eid_prefix (struct eid * eid, prefix_t * prefix);


/* Main LISP Thread Instance */
int set_lisp_mapserver (struct sockaddr_storage mapsrv);
int set_lisp_locator (struct locator * loc);

int unset_lisp_mapserver (void);
int unset_lisp_locator (struct sockaddr_storage loc_addr);

void start_lisp_thread (pthread_t * tid, void * (* thread) (void * param));	

void * lisp_map_register_thread (void * param);	/* register EIDs periodically	*/
void * lisp_map_reply_thread (void * param);	/* process map reply thread	*/
void * lisp_dp_thread (void * param);		/* recv LISP encaped DP packet	*/

#endif /* _INSTANCE_H_*/
