#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "control.h"
#include "instance.h"
#include "sockaddrmacro.h"

list_t *
install_cmd_node (void)
{
	list_t * cmd_tuple;
	
	cmd_tuple = create_list ();

	/* install map server configuration node */
	struct cmd_node * map;
	map = (struct cmd_node *) malloc (sizeof (struct cmd_node));
	memset (map, 0, sizeof (struct cmd_node));
	map->depth = 1;
	strncpy (map->init_cmd, "mapserver", INIT_CMD_MAX_LEN);
	map->func = config_map_server;
	append_listnode (cmd_tuple, map);

	/* install locator configuration node */
	struct cmd_node * loc;
	loc = (struct cmd_node *) malloc (sizeof (struct cmd_node));
	memset (loc, 0, sizeof (struct cmd_node));
	loc->depth = 1;
	strncpy (loc->init_cmd, "locator", INIT_CMD_MAX_LEN);
	loc->func = config_locator;
	append_listnode (cmd_tuple, loc);

	/* install EID configuration node */
	struct cmd_node * eid;
	eid = (struct cmd_node *) malloc (sizeof (struct cmd_node));
	memset (eid, 0, sizeof (struct cmd_node));
	eid->depth = 1;
	strncpy (eid->init_cmd, "eid", INIT_CMD_MAX_LEN);
	eid->func = config_eid;
	append_listnode (cmd_tuple, eid);


	return cmd_tuple;
}

int
compare_cmd_node (void * pa, void * pb)
{
	char * cmd = (char *) pa;
	struct cmd_node * cnode = (struct cmd_node *) pb;

	return strcmp (cmd, cnode->init_cmd);
}

struct cmd_node *
search_cmd_node (list_t * cmd_tuple, char * cmd)
{
	return search_listnode (cmd_tuple, cmd, compare_cmd_node);
}

/* Configure Map Server Address */
enum return_type 
cmd_set_map_server (char ** args)
{
	char * mapsrvcaddr = args[1];
	struct sockaddr_storage mapsrvaddr;
	struct addrinfo hints, * res;

	/* set map server address */
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo (mapsrvcaddr, LISP_CONTROL_CPORT, &hints, &res) != 0) 
		return SET_MAPSERVER_FAILED_INVALID_ADDRESS;
	
	memcpy (&mapsrvaddr, res->ai_addr, res->ai_addrlen);
	freeaddrinfo (res);

	if (set_lisp_mapserver (mapsrvaddr) != 0) 
		return SET_MAPSERVER_FAILED_INVALID_ADDRESS;

	return SET_MAPSERVER_SUCCESS;
}

enum return_type 
config_map_server (char ** args)
{
	char * action = args[2];

	if (action != NULL) 
		return cmd_set_map_server (args);
	else if (strcmp (action, "delete") == 0) 
		return unset_lisp_mapserver ();
	else 
		return INVALID_COMMAND;

	return INVALID_COMMAND;
}


/* Configure Locators */

int
compare_locator (void * p, void * d)
{
	struct locator * ploc, * dloc;

	ploc = (struct locator *) p;
	dloc = (struct locator *) d;

	if (COMPARE_SOCKADDR (&(ploc->loc_addr), &(dloc->loc_addr))) 
		return 0;

	return -1;
}

struct locator *
search_locator (struct sockaddr_storage loc_addr)
{
	struct locator ploc;

	ploc.loc_addr = loc_addr;
	
	return search_listnode (lisp.loc_tuple, &ploc, compare_locator);
}

enum return_type
cmd_set_locator (char ** args)
{
	char * mapsrvcaddr = args[1];
	struct locator loc;
	struct addrinfo hints, * res;
	
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo (mapsrvcaddr, LISP_CONTROL_CPORT, &hints, &res) != 0) 
		return SET_LOCATOR_FAILED_INVALID_ADDRESS;

	memcpy (&(loc.loc_addr), res->ai_addr, res->ai_addrlen);
	freeaddrinfo (res);

	if (search_locator (loc.loc_addr) != NULL) 
		return SET_LOCATOR_FAILED_LOCATOR_EXISTS;

	if (set_lisp_locator (loc) != 0) 
		return SET_LOCATOR_FAILED;

	return SET_LOCATOR_SUCCESS;
}


enum return_type
cmd_unset_locator (char ** args)
{
	char * caddr = args[1];
	struct addrinfo hints, * res;
	struct sockaddr_storage loc_addr;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo (caddr, LISP_CONTROL_CPORT, &hints, &res) != 0)
		return UNSET_LOCATOR_FAILED_INVALID_ADDRESS;

	memcpy (&loc_addr, res->ai_addr, res->ai_addrlen);

	if (unset_lisp_locator (loc_addr) != 0) {
		freeaddrinfo (res);
		return UNSET_LOCATOR_FAILED_DOES_NOT_EXISTS;
	}
	
	freeaddrinfo (res);

	return UNSET_LOCATOR_SUCCESS;
}

enum return_type
cmd_set_locator_paramater (char ** args)
{
	char * caddr = args[1];
	char * param = args[2];
	char * value = args[3];
	struct locator * loc;
	struct addrinfo hints, * res;
	struct sockaddr_storage loc_addr;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo (caddr, LISP_CONTROL_CPORT, &hints, &res) != 0)
		return SET_LOCATOR_PARAM_FAILED_INVALID_ADDRESS;

	freeaddrinfo (res);
	memcpy (&loc_addr, res->ai_addr, res->ai_addrlen);

	if ((loc = search_locator (loc_addr)) == NULL) {
		return SET_LOCATOR_PARAM_FAILED_DOES_NOT_EXISTS;
	}

	if (param == NULL || value == NULL) 
		return INVALID_COMMAND;

	if (strcmp (param, "priority") == 0) 
		loc->priority = atoi (value);
	else if (strcmp (param, "weight") == 0) 
		loc->weight = atoi (value);
	else if (strcmp (param, "m_priority") == 0) 
		loc->m_priority = atoi (value);
	else if (strcmp (param, "m_weight") == 0) 
		loc->m_weight = atoi (value);
	else 
		return INVALID_COMMAND;

	return SET_LOCATOR_PARAM_SUCCESS;
}

enum return_type
config_locator (char ** args)
{

	/* that is create locator */
	if (args[2] == NULL) 
		return cmd_set_locator (args);


	/* that is delete locator only */
	if (strcmp (args[2], "delete") != 0) 
		return cmd_unset_locator (args);


	/* else if configure paramater */
	return cmd_set_locator_paramater (args);
}


enum return_type cmd_create_eid (char ** args);
enum return_type cmd_set_eid_interface (char ** args);
enum return_type cmd_set_eid_authentication_key (char ** args);
enum return_type cmd_set_eid_prefix (char ** args);
