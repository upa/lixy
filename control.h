#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "list/list.h"

#define CONTROL_MSG_BUF_LEN	1024
#define CONTROL_ARGS_MAX	8
#define CONTROL_MSG_MAX_LEN	512

/* 
 * All of return value of configuration commands 
 * return messages are defined in install_control_message
*/
enum return_type {
	SUCCESS,
	SUCCESS_NO_MESSAGE,
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
	ERR_AUTHKEY_TOO_LONG,

	RETURN_TYPE_MAX			/* sentinel */
};

#define GET_CONTROL_MSG(TYPE) lisp.ctl_message[(TYPE)]
#define GET_CONTROL_MSG_LEN(TYPE) (strlen (GET_CONTROL_MSG((TYPE))) + 1)
#define WRITE_CONTROL_MSG(SOCKET, TYPE) \
	write ((SOCKET), GET_CONTROL_MSG((TYPE)), GET_CONTROL_MSG_LEN(TYPE) + 1)


/*
 * lixyctl [element] [value] [action]
 *	   0	     1		 2 
 * lixyctl mapserver [v4/v6_prefix|delete]
 * lixyctl locator   [v4/v6_prefix] [priority|weight|m_prioritu|m_weight|create|delete]
 * lixyctl eid 	     eid_name 	 [create|delete]
 * lixyctl eid 	     eid_name 	 authkey [keystring]
 * lixyctl eid 	     eid_name 	 prefix [v4/v6_prefix] [delete]
 *
 * lixyctl show      ipv4-route  [active|negative|drop|queried|static]
 * lixyctl show      ipv6-route  [active|negative|drop|queried|static]
 * lixyctl show      eid         [eid name]
 * lixyctl show      map-server
 * lixyctl show      locator
 *
 * lixyctl route     [ipv4|ipv6]    [dst prefix]   [ITR|delete]
 *
 */

#define INIT_CMD_MAX_LEN 16

struct cmd_node {
	int depth;
	char init_cmd[INIT_CMD_MAX_LEN];
	enum return_type (* func) (int socket, char **);
};

/* actually executed commands */
enum return_type config_map_server (int socket, char ** args);
enum return_type config_locator (int socket, char ** args);
enum return_type config_eid (int socket, char ** args);
enum return_type config_show (int socket, char ** args);
enum return_type config_route (int socket, char ** args);

list_t * install_cmd_node (void);
char ** install_control_message (void);
void * lisp_op_thread (void * param);


#endif /* _CONTROL_H_ */
