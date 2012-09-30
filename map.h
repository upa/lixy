#ifndef _MAP_H_
#define _MAP_H_

#include "common.h"


/* Source EID AFI and M bit are always set to 0 */
int set_lisp_map_request (char * buf, int len, prefix_t * prefix);

/* Always Record count is 1, Locator count is 1 */
int set_lisp_map_register (char * buf, int len, struct eid * eid);

int process_lisp_map_reply (struct lisp_map_reply * maprep);





#endif /* _MAP_H_ */
