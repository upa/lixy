#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "control.h"
#include "instance.h"
#include "sockaddrmacro.h"
#include "error.h"
#include "maptable.h"

#define WRITE_SOCKET(sock, str) write (sock, str, strlen (str) + 1)

int
strsplit (char * str, char ** argv, int maxnum)
{
        int argc;
        char * c;

        for (argc = 0, c = str; *c == ' '; c++);
        while (*c && argc < maxnum) {
                argv[argc++] = c;
                while (*c && *c > ' ') c++;
                while (*c && *c <= ' ') *c++ = '\0';
        }

	if (argc >= maxnum)
		return -1;

        return argc;
}


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

	/* install show command node */
	struct cmd_node * show;
	show = (struct cmd_node *) malloc (sizeof (struct cmd_node));
	memset (show, 0, sizeof (struct cmd_node));
	show->depth = 1;
	strncpy (eid->init_cmd, "show", INIT_CMD_MAX_LEN);
	show->func = config_show;
	append_listnode (cmd_tuple, show);

	return cmd_tuple;
}


char **
install_control_message (void)
{
	int n;
	char ** ptr;

	ptr = (char **) malloc (sizeof (char *) * RETURN_TYPE_MAX);
	memset (ptr, 0, sizeof (char *) * RETURN_TYPE_MAX);
	for (n = 0; n < RETURN_TYPE_MAX; n++) {
		ptr[n] = (char *) malloc (CONTROL_MSG_MAX_LEN);
		memset (ptr[n], 0, sizeof (CONTROL_MSG_MAX_LEN));
		ptr[n][0] = '\0';
	}

#define SET_CONTROL_MSG(PTR, NUM, MSG)		\
	strncpy ((PTR[(NUM)]), (MSG), CONTROL_MSG_MAX_LEN);

	SET_CONTROL_MSG (ptr, SUCCESS, "success");
	SET_CONTROL_MSG (ptr, SUCCESS_NO_MESSAGE, "");
	SET_CONTROL_MSG (ptr, ERR_FAILED, "failed");
	SET_CONTROL_MSG (ptr, ERR_INVALID_COMMAND, "invalid command");
	SET_CONTROL_MSG (ptr, ERR_INVALID_ADDRESS, "invalid address");
	SET_CONTROL_MSG (ptr, ERR_EID_EXISTS, "EID exists");
	SET_CONTROL_MSG (ptr, ERR_LOCATOR_EXISTS, "Locator exists");
	SET_CONTROL_MSG (ptr, ERR_INTERFACE_EXISTS, "Interface exists");
	SET_CONTROL_MSG (ptr, ERR_ADDRESS_EXISTS, "Address exists");
	SET_CONTROL_MSG (ptr, ERR_EID_DOES_NOT_EXISTS, "EID does not exists");
	SET_CONTROL_MSG (ptr, ERR_LOCATOR_DOES_NOT_EXISTS, "Locator does not exists");
	SET_CONTROL_MSG (ptr, ERR_INTERFACE_DOES_NOT_EXISTS, "Interface doesnot exists");
	SET_CONTROL_MSG (ptr, ERR_ADDRESS_DOES_NOT_EXISTS, "Address does not exists");
	SET_CONTROL_MSG (ptr, ERR_AUTHKEY_TOO_LONG, "authentication key is too long");
	
	return ptr;
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


void *
lisp_op_thread (void * param)
{
	/* Process control command */

	int n, accept_socket;
	char buf[CONTROL_MSG_BUF_LEN];
	char * args[CONTROL_ARGS_MAX];
	struct cmd_node * cnode;

	if (listen (lisp.cmd_socket, 1) < 0)
		error_quit ("%s: listen_failed", __func__);

	while (1) {
		for (n = 0; n < CONTROL_ARGS_MAX; n++) 
			args[n] = NULL;
		memset (buf, 0, sizeof (buf));

		accept_socket = accept (lisp.cmd_socket, NULL, 0);

		if (read (accept_socket, buf, sizeof (buf)) < 0) {
			error_warn ("%s: read(2) control socket failed", __func__);
			shutdown (accept_socket, 1);
			close (accept_socket);
			continue;
		}
		
		if (strsplit (buf, args, CONTROL_ARGS_MAX) < 0) {
			error_warn ("%s: parse command failed", __func__);
			WRITE_CONTROL_MSG (accept_socket, ERR_INVALID_COMMAND);
			shutdown (accept_socket, 1);
			close (accept_socket);
			continue;
		}
		
		if ((cnode = search_cmd_node (lisp.cmd_tuple, args[0])) == NULL) {
			error_warn ("%s: invalid command %s", __func__, args[0]);
			WRITE_CONTROL_MSG (accept_socket, ERR_INVALID_COMMAND);
			shutdown (accept_socket, 1);
			close (accept_socket);
			continue;
		}
			
		n = cnode->func (accept_socket, args);
		WRITE_CONTROL_MSG (accept_socket, n);
		shutdown (accept_socket, 1);
		close (accept_socket);
	}

	return NULL;
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
		return ERR_INVALID_ADDRESS;
	
	memcpy (&mapsrvaddr, res->ai_addr, res->ai_addrlen);
	freeaddrinfo (res);

	if (set_lisp_mapserver (mapsrvaddr) != 0) 
		return ERR_INVALID_ADDRESS;

	return SUCCESS;
}

enum return_type 
config_map_server (int socket, char ** args)
{
	char * action = args[2];

	if (action != NULL) 
		return cmd_set_map_server (args);
	else if (strcmp (action, "delete") == 0) 
		return unset_lisp_mapserver ();
	else 
		return ERR_INVALID_COMMAND;

	return ERR_INVALID_COMMAND;
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
cmd_create_locator (char ** args)
{
	char * mapsrvcaddr = args[1];
	struct locator loc;
	struct addrinfo hints, * res;
	
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo (mapsrvcaddr, LISP_CONTROL_CPORT, &hints, &res) != 0) 
		return ERR_INVALID_ADDRESS;

	memcpy (&(loc.loc_addr), res->ai_addr, res->ai_addrlen);
	freeaddrinfo (res);

	if (search_locator (loc.loc_addr) != NULL) 
		return ERR_LOCATOR_EXISTS;

	if (set_lisp_locator (loc) != 0) 
		return ERR_FAILED;

	return SUCCESS;
}


enum return_type
cmd_delete_locator (char ** args)
{
	char * caddr = args[1];
	struct addrinfo hints, * res;
	struct sockaddr_storage loc_addr;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo (caddr, LISP_CONTROL_CPORT, &hints, &res) != 0)
		return ERR_INVALID_ADDRESS;

	memcpy (&loc_addr, res->ai_addr, res->ai_addrlen);

	if (unset_lisp_locator (loc_addr) != 0) {
		freeaddrinfo (res);
		return ERR_LOCATOR_DOES_NOT_EXISTS;
	}
	
	freeaddrinfo (res);

	return SUCCESS;
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
		return ERR_INVALID_ADDRESS;

	freeaddrinfo (res);
	memcpy (&loc_addr, res->ai_addr, res->ai_addrlen);

	if ((loc = search_locator (loc_addr)) == NULL) {
		return ERR_LOCATOR_DOES_NOT_EXISTS;
	}

	if (param == NULL || value == NULL) 
		return ERR_INVALID_COMMAND;

	if (strcmp (param, "priority") == 0) 
		loc->priority = atoi (value);
	else if (strcmp (param, "weight") == 0) 
		loc->weight = atoi (value);
	else if (strcmp (param, "m_priority") == 0) 
		loc->m_priority = atoi (value);
	else if (strcmp (param, "m_weight") == 0) 
		loc->m_weight = atoi (value);
	else 
		return ERR_INVALID_COMMAND;

	return SUCCESS;
}

enum return_type
config_locator (int socket, char ** args)
{

	/* that is create locator */
	if (args[2] == NULL) 
		return cmd_create_locator (args);


	/* that is delete locator only */
	if (strcmp (args[2], "delete") != 0) 
		return cmd_delete_locator (args);


	/* else if configure paramater */
	return cmd_set_locator_paramater (args);
}



/* Configure EID functions */

int
compare_eid (void * n, void * e)
{
	char * name = n;
	struct eid * eid = e;

	return strncmp (name, eid->name, LISP_EID_NAME_LEN);
}



struct eid *
search_eid (char * eid_name)
{
	return search_listnode (lisp.eid_tuple, eid_name, compare_eid);
}


enum return_type 
cmd_create_eid (char ** args)
{
	char * eid_name = args[1];
	struct eid * eid;
	
	if (search_eid (eid_name) != NULL)
		return ERR_EID_EXISTS;

	eid = create_eid_instance (eid_name);
	append_listnode (lisp.eid_tuple, eid);

	return SUCCESS;
}


enum return_type 
cmd_delete_eid (char ** args)
{
	char * eid_name = args[1];
	struct eid * eid;

	if ((eid = search_eid (eid_name)) == NULL) 
		return ERR_EID_DOES_NOT_EXISTS;
	
	delete_eid_instance (eid);
	delete_listnode (lisp.eid_tuple, eid);
	free (eid);

	return SUCCESS;
}


enum return_type 
cmd_set_eid_interface (char ** args)
{
	char * eid_name = args[1];
	char * ifname = args[2];
	struct eid * eid;

	if (if_nametoindex (ifname) == 0) 
		return ERR_INTERFACE_DOES_NOT_EXISTS;

	if ((eid = search_eid (eid_name)) == NULL) 
		return ERR_EID_DOES_NOT_EXISTS;

	if (set_eid_iface (eid, ifname) < 0)
		return ERR_FAILED;
	
	return SUCCESS;
}


enum return_type 
cmd_unset_eid_interface (char ** args)
{
	char * eid_name = args[1];
	struct eid * eid;

	if ((eid = search_eid (eid_name)) == NULL)
		return ERR_EID_DOES_NOT_EXISTS;

	if (unset_eid_iface (eid) != 0)
		return ERR_FAILED;

	return SUCCESS;
}

enum return_type 
cmd_set_eid_authentication_key (char ** args)
{
	char * eid_name = args[1];
	char * key = args[2];
	struct eid * eid;

	if ((eid = search_eid (eid_name)) == NULL)
		return ERR_EID_DOES_NOT_EXISTS;

	if (strlen (key) > LISP_MAX_KEYLEN)
		return ERR_AUTHKEY_TOO_LONG;

	memcpy (eid->authkey, key, strlen (key) + 1);

	return SUCCESS;
}

enum return_type 
cmd_unset_eid_authentication_key (char ** args)
{
	char * eid_name = args[1];
	struct eid * eid;

	if ((eid = search_eid (eid_name)) == NULL)
		return ERR_EID_DOES_NOT_EXISTS;

	if (unset_eid_authkey (eid) != 0) 
		return ERR_FAILED;

	return SUCCESS;
}

int
compare_prefix (void * d1, void * d2)
{
	prefix_t * p1 = d1;
	prefix_t * p2 = d2;

	if (p1->family != p2->family)
		return -1;

	if (p1->bitlen != p2->bitlen)
		return -1;

	switch (p1->family) {
	case AF_INET :
		if (COMPARE_SADDR_IN (p1->add.sin, p2->add.sin))
			return 0;
		break;
	case AF_INET6 :
		if (COMPARE_SADDR_IN6 (p1->add.sin6, p2->add.sin6))
			return 0;
		break;
	}

	return -1;
}

prefix_t *
search_prefix (list_t * list, prefix_t * prefix)
{
	return search_listnode (list, prefix, compare_prefix);
}

enum return_type 
cmd_set_eid_prefix (char ** args)
{
	char * eid_name = args[1];
	char * c_prefix = args[3];
	prefix_t * prefix;
	struct addrinfo hints, * res;
	struct eid * eid;

	if ((eid = search_eid (eid_name)) == NULL)
		return ERR_EID_DOES_NOT_EXISTS;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	
	if (getaddrinfo (c_prefix, LISP_CONTROL_CPORT, &hints, &res) != 0)
		return ERR_INVALID_ADDRESS;

	switch (res->ai_family) {
	case AF_INET :
		prefix = ascii2prefix (AF_INET, c_prefix);
		break;
	case AF_INET6 :
		prefix = ascii2prefix (AF_INET6, c_prefix);
		break;
	default :
		return ERR_INVALID_ADDRESS;
		break;
	}

	if (prefix == NULL)
		return ERR_INVALID_ADDRESS;

	if (search_prefix (eid->prefix_tuple, prefix) != NULL)
		return ERR_ADDRESS_EXISTS;
	
	append_listnode (eid->prefix_tuple, prefix);
	
	return SUCCESS;
}

enum return_type 
cmd_unset_eid_prefix (char ** args)
{
	char * eid_name = args[1];
	char * c_prefix = args[3];
	prefix_t * prefix, * pprefix;
	struct addrinfo hints, * res;
	struct eid * eid;

	if ((eid = search_eid (eid_name)) == NULL)
		return ERR_EID_DOES_NOT_EXISTS;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	
	if (getaddrinfo (c_prefix, LISP_CONTROL_CPORT, &hints, &res) != 0)
		return ERR_INVALID_ADDRESS;

	switch (res->ai_family) {
	case AF_INET :
		prefix = ascii2prefix (AF_INET, c_prefix);
		break;
	case AF_INET6 :
		prefix = ascii2prefix (AF_INET6, c_prefix);
		break;
	default :
		freeaddrinfo (res);
		return ERR_INVALID_ADDRESS;
		break;
	}

	freeaddrinfo (res);

	if (prefix == NULL)
		return ERR_INVALID_ADDRESS;

	if ((pprefix = search_prefix (eid->prefix_tuple, prefix)) == NULL) {
		free (prefix);
		return ERR_ADDRESS_DOES_NOT_EXISTS;
	}
	
	delete_listnode (eid->prefix_tuple, pprefix);
	free (prefix);
	free (pprefix);

	return SUCCESS;
}


enum return_type 
config_eid (int socket, char ** args)
{
	char * action = args[2];
	
	if (strcmp (action, "create") == 0) {
		return cmd_create_eid (args);
	}

	else if (strcmp (action, "delete") == 0) {
		return cmd_delete_eid (args);
	}

	else if (strcmp (action, "authkey") == 0) {
		return cmd_set_eid_authentication_key (args);
	}

	else if (strcmp (action, "interface") == 0) {
		cmd_unset_eid_interface (args);
		return cmd_set_eid_interface (args);
	}

	else if (strcmp (action, "prefix") == 0) {
		char * delete = args[4];
		if (delete != NULL) {
			if (strcmp (delete, "delete") == 0)
				return cmd_unset_eid_prefix (args);
		} else {
			return cmd_set_eid_prefix (args);
		}
	}

	return ERR_INVALID_COMMAND;
}

/* show parameter functions */

enum return_type
cmd_show_route (int af, int socket, char ** args)
{
	int state;
	char * type = args[2];
	char buf[CONTROL_MSG_BUF_LEN], addrbuf1[52], addrbuf2[52];
	char * mapstate_string[] = MAPSTATE_STRING_INIT ();
	patricia_node_t * pn;
	struct mapnode * mn;


	if (type == NULL) 
		state = -1;

	else if (strcmp (type, "active")) 
		state = MAPSTATE_ACTIVE;
	
	else if (strcmp (type, "negative")) 
		state = MAPSTATE_NEGATIVE;
	
	else if (strcmp (type, "drop")) 
		state = MAPSTATE_DROP;
	
	else if (strcmp (type, "queried")) 
		state = MAPSTATE_QUERIED;

	else if (strcmp (type, "static")) 
		state = MAPSTATE_STATIC;



	PATRICIA_WALK (MAPTABLE_TREE_HEAD (lisp.rib), pn) {
		mn = (struct mapnode *) (pn->data);
		if (state < 0 || state == mn->state) {

			if (pn->prefix->family != af) 
				PATRICIA_WALK_BREAK;

			/* extract destination prefix from prefix_t*/
			inet_ntop (pn->prefix->family, &(pn->prefix->add), 
				   addrbuf1, sizeof (addrbuf1));

			/* extract ETR address from sockaddr_storage*/
			switch (EXTRACT_FAMILY (mn->addr)) {
			case AF_INET :
				inet_ntop (AF_INET, &(EXTRACT_INADDR (mn->addr)), 
					   addrbuf2, sizeof (addrbuf2));
				break;
			case AF_INET6 :
				inet_ntop (AF_INET6, &(EXTRACT_IN6ADDR (mn->addr)), 
					   addrbuf2, sizeof (addrbuf2));
				break;
			}
			

			snprintf (buf, sizeof (buf), "%s/%d %s %s\n", 
				  addrbuf1, pn->prefix->bitlen, addrbuf2, 
				  mapstate_string[mn->state]);
			WRITE_SOCKET (socket, buf);
		} 
	} PATRICIA_WALK_END;
	
	
	return SUCCESS_NO_MESSAGE;
}


void
cmd_write_eid (int socket, struct eid * eid)
{
	char buf[CONTROL_MSG_BUF_LEN];
	char addrbuf[52];
	listnode_t * ln;
	prefix_t * prefix;

	memset (buf, 0, sizeof (buf));

	snprintf (buf, sizeof (buf), 
		  "Name : %s\n"
		  "   Inteface : %s\n"
		  "   Auth Key : %s\n"
		  "   Prefixes :", 
		  eid->name, eid->ifname, eid->authkey);

	MYLIST_FOREACH (eid->prefix_tuple, ln) {
		prefix = (prefix_t *) (ln->data);
		inet_ntop (prefix->family, &(prefix->add), addrbuf, sizeof (addrbuf));
		snprintf (buf, sizeof (buf), "%s %s", buf, addrbuf);
	}
	
	snprintf (buf, sizeof (buf), "%s\n\n", buf);

	WRITE_SOCKET (socket, buf);

	return;
}

enum return_type
cmd_show_eid (int socket, char ** args)
{
	char * eid_name = args[2];
	listnode_t * ln;
	struct eid * eid;

	if (eid_name != NULL) {
		if ((eid = search_eid (eid_name)) == NULL)
			return ERR_EID_DOES_NOT_EXISTS;

		cmd_write_eid (socket, eid);
	} else {
		MYLIST_FOREACH (lisp.eid_tuple, ln) {
			eid = (struct eid *) (ln->data);
			cmd_write_eid (socket, eid);
		}
	}

	return SUCCESS_NO_MESSAGE;
}
	
enum return_type
cmd_show_mapserver (int socket, char ** args)
{
	char buf[CONTROL_MSG_BUF_LEN], addrbuf[52];
	
	memset (buf, 0, sizeof (buf));
	memset (addrbuf, 0, sizeof (addrbuf));

	switch (EXTRACT_FAMILY (lisp.mapsrvaddr)) {
	case AF_INET :
		inet_ntop (AF_INET, &(EXTRACT_INADDR (lisp.mapsrvaddr)), 
				      addrbuf, sizeof (addrbuf));
		snprintf (buf, sizeof (buf), "map server address : %s\n", addrbuf);
		break;

	case AF_INET6 :
		inet_ntop (AF_INET6, &(EXTRACT_IN6ADDR (lisp.mapsrvaddr)), 
				      addrbuf, sizeof (addrbuf));
		snprintf (buf, sizeof (buf), "map server address : %s\n", addrbuf);
		break;

	default :
		snprintf (buf, sizeof (buf), "map server is node defined\n");
		break;
	}
	
	WRITE_SOCKET (socket, buf);

	return SUCCESS_NO_MESSAGE;
}



enum return_type
config_show (int socket, char ** args)
{
	char * action = args[2];

	if (strcmp (action, "ipv4-route") == 0) {
		return cmd_show_route (AF_INET6, socket, args);
	}
	
	else if (strcmp (action, "ipv6-route") == 0) {
		return cmd_show_route (AF_INET6, socket, args);
	}

	else if (strcmp (action, "eid") == 0) {
		return cmd_show_eid (socket, args);
	}

	else if (strcmp (action, "map-server") == 0) {
		return cmd_show_mapserver (socket, args);
	}

	return ERR_INVALID_COMMAND;
}



/* install/uninstall static route*/
enum return_type
config_route (int socket, char ** args)
{
	char * action = args[2];

	if (strcmp (action, "delete") == 0) {
		return cmd_delete_route (args);
	} else {
		return cmd_isntall_route (args);
	}

	return ERR_INVALID_COMMAND;
}


enum return_type
cmd_delete_route (args)
{
	
}
