#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "list/list.h"

/* All of return value of configuration commands */
enum return_type {
	INVALID_COMMAND,

	SET_MAPSERVER_SUCCESS,
	SET_MAPSERVER_FAILED,
	SET_MAPSERVER_FAILED_INVALID_ADDRESS,

	SET_LOCATOR_SUCCESS,
	SET_LOCATOR_FAILED,
	SET_LOCATOR_FAILED_LOCATOR_EXISTS,
	SET_LOCATOR_FAILED_INVALID_ADDRESS,

	SET_LOCATOR_PARAM_SUCCESS,
	SET_LOCATOR_PARAM_FAILED_INVALID_ADDRESS,
	SET_LOCATOR_PARAM_FAILED_DOES_NOT_EXISTS,

	UNSET_LOCATOR_SUCCESS,
	UNSET_LOCATOR_FAILED_INVALID_ADDRESS,
	UNSET_LOCATOR_FAILED_DOES_NOT_EXISTS,

	CREATE_EID_SUCCESS,
	CREATE_EID_FAILED,
	CREATE_EID_FAILED_EXSITS,

	SET_EID_INTERFACE_SUCCESS,
	SET_EID_INTERFACE_FAILED,
	SET_EID_AUTH_KEY_SUCCESS,
	SET_EID_AUTH_KEY_FAILED,
	SET_EID_AUTH_KEY_FAILED_TOO_LONG,
	SET_EID_PREFIX_SUCCESS,
	SET_EID_PREFIX_FAILED,
	SET_EID_PREFIX_FAILED_INVALID_PREFIX
};




/*
 * lixyctl [element] [value] [action]
 *
 *	lixyctl mapserver [v4/v6_prefix] [delete]
 *	lixyctl locator [v4/v6_prefix] [priority|weight|m_prioritu|m_weight|delete]
 *	lixyctl eid eid_name [create|delete]
 *	lixyctl eid eid_name authkey [keystring]
 *	lixyctl eid eid_name interface [interface name]
 *	lixyctl eid eid_name prefix [v4/v6_prefix]
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
