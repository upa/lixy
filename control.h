#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "list/list.h"

/* All of return value of configuration commands */
enum return_type {
	SUCCESS,

	ERR_FAILED,
	ERR_INVALID_COMMAND,
	ERR_INVALID_ADDRESS,

	ERR_EID_EXISTS,
	ERR_LOCATOR_EXISTS,
	ERR_INTERFACE_EXISTS,
	ERR_ADDRESS_EXISTS,

	ERR_EID_DOES_NOT_EXISTS,
	ERR_LOCATOR_DOES_NOT_EXISTS,
	ERR_INTERFACE_DOES_NOT_EXISTS,
	ERR_ADDRESS_DOES_NOT_EXISTS,

	ERR_AUTHKEY_TOO_LONG
};




/*
 * lixyctl [element] [value] [action]
 *		0	  1		 2 
 *	lixyctl mapserver [v4/v6_prefix] [delete]
 *	lixyctl locator   [v4/v6_prefix] [priority|weight|m_prioritu|m_weight|delete]
 *	lixyctl eid 	  eid_name 	 [create|delete]
 *	lixyctl eid 	  eid_name 	 authkey [keystring]
 *	lixyctl eid 	  eid_name 	 interface [interface name]
 *	lixyctl eid 	  eid_name 	 prefix [v4/v6_prefix] [delete]
 */

#define INIT_CMD_MAX_LEN 16

struct cmd_node {
	int depth;
	char init_cmd[INIT_CMD_MAX_LEN];
	enum return_type (* func) (char **);
};

/* actually executed commands */
enum return_type config_map_server (char ** args);
enum return_type config_locator (char ** args);
enum return_type config_eid (char ** args);



list_t * install_cmd_node (void);
void * lisp_op_thread (void * param);


#endif /* _CONTROL_H_ */
