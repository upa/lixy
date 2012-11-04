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
int set_lisp_locator (struct locator loc);

int unset_lisp_mapserver (void);
int unset_lisp_locator (struct sockaddr_storage loc_addr);

void start_lisp_thread (pthread_t * tid, void * (* thread) (void * param));	

void * lisp_map_register_thread (void * param);	/* register EID periodically */
void * lisp_map_message_thread (void * param);	/* process message thread */
void * lisp_dp_thread (void * param);		/* recv LISP DP packet	*/
void * lisp_dp_tun_thread (void * param);	/* send LISP DP packet	*/
void * lisp_raw_packet_thread (void * param);	/* watich raw socket */

/* netlink functions */
int is_route_scope_link (int af, void * addr);
int add_route_to_tun (int af, void * addr, int prefixlen);
int del_route_to_tun (int af, void * addr, int prefixlen);

#endif /* _INSTANCE_H_*/
