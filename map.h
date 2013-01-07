#ifndef _MAP_H_
#define _MAP_H_

#include "common.h"

#define LISP_MSG_BUF_LEN 1024

/* Source EID AFI and M bit are always set to 0 */
int set_lisp_map_request (char * buf, int len, prefix_t * prefix);

/* Always Record count is 1, Locator count is 1 */
int set_lisp_map_register (char * buf, int len, prefix_t * prefix, char * key);

/* in this implementation, map reply is used for RLOC Probing */
int set_lisp_map_reply (char * buf, int len, prefix_t * prefix, 
			u_int32_t * nonce);


int send_map_request (prefix_t * prefix);
int send_map_register (struct eid * eid);
int send_map_reply (prefix_t * prefix, u_int32_t * nonce,
		    struct sockaddr_storage mapsrvaddr);

void set_nonce (void * ptr, int size);




/* Porccese Message Packet Fun*/

void process_lisp_map_message (char * pkt);

/* pkt is pointer to lisp_map_reply */
int process_lisp_map_reply (char * pkt);    

/* pkt is pointer to lisp_map_request */
int process_lisp_map_request (char * pkt);


#endif /* _MAP_H_ */
