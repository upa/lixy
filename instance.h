#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#include "common.h"

#define LISP_EID_DP_BUF_LEN	2048

struct eid * create_eid_instance (char * eidname);
struct eid * delete_eid_instance (struct eid * eid);

int set_eid_iface (struct eid * eid, char * ifname);
int set_eid_authkey (struct eid * eid, char * authkey);
int set_eid_prefix (struct eid * eid, prefix_t * prefix);

int unset_eid_iface (struct eid * eid);
int unset_eid_authkey (struct eid * eid);
int unset_eid_prefix (struct eid * eid, prefix_t * prefix);

#endif /* _INSTANCE_H_*/
