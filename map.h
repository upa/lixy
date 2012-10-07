#ifndef _MAP_H_
#define _MAP_H_

#include "common.h"

#define LISP_MSG_BUF_LEN 1024

/* Source EID AFI and M bit are always set to 0 */
int set_lisp_map_request (char * buf, int len, prefix_t * prefix);

/* Always Record count is 1, Locator count is 1 */
int set_lisp_map_register (char * buf, int len, prefix_t * prefix, struct eid * eid);

int process_lisp_map_reply (struct lisp_map_reply * maprep);


int send_map_request (prefix_t * prefix);
int send_map_register (struct eid * eid);


#endif /* _MAP_H_ */
