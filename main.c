#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <linux/if_packet.h>

#include "common.h"
#include "iftun.h"
#include "error.h"
#include "instance.h"
#include "control.h"

int 
create_udp_socket (int port)
{
	int sock;

	struct sockaddr_in6 saddr_in6;

	/* IPv4 is supported by Mapped Address */

	if ((sock = socket (AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err (EXIT_FAILURE, "can not create data plane udp socket");
	
	memset (&saddr_in6, 0, sizeof (saddr_in6));
	saddr_in6.sin6_family = AF_INET6;
	saddr_in6.sin6_port = htons (port);

	if (bind (sock, 
		  (struct sockaddr *) &saddr_in6, 
		  sizeof (saddr_in6)) < 0)
		err (EXIT_FAILURE, "can not bind date plane udp socket");
#if 0
	int off = 0;
	if (setsockopt (sock, IPPROTO_IPV6, IPV6_V6ONLY, 
			&off, sizeof (off)) != 0)
		err (EXIT_FAILURE, "can not set IPV6_V6ONLY off");
#endif

	return sock;
}


int 
create_lisp_udp_socket (void)
{
	int sock, on = 1;

	sock = create_udp_socket (LISP_DATA_PORT);
	
	if (setsockopt (sock, SOL_SOCKET, SO_NO_CHECK,
			&on, sizeof (on)) != 0)
		err (EXIT_FAILURE, "can not set SO_NO_CHECK");


	return sock;
}

int 
create_lisp_ctl_socket (void)
{
	return create_udp_socket (LISP_CONTROL_PORT);
}

int 
create_lisp_raw_socket (int af)
{
	int sock;
	int on = 1;

#ifdef linux
	if ((sock = socket (af, SOCK_RAW, IPPROTO_RAW)) < 0)
		err (EXIT_FAILURE, "can not create "
		     "raw socket for send packet");

	switch (af) {
	case AF_INET :
		if (setsockopt (sock, IPPROTO_IP, IP_HDRINCL, 
				&on, sizeof (on)) != 0)
			err (EXIT_FAILURE, "can not set sock opt "
			     "IPv4 HDRINCL");
		break;
#if 0
	case AF_INET6 :
		if (setsockopt (sock, IPPROTO_IPV6, IPV6_HDRINCL, 
				&on, sizeof (on)) != 0)
			err (EXIT_FAILURE, "can not set sock opt "
			     "IPv6 HDRINCL");
		break;
#endif
	}
#endif

	return sock;
}

int
create_raw_socket (void)
{
	int sock;
	struct sockaddr_ll saddr_ll;

	if ((sock = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 1) {
		error_warn ("%s: can not create raw socket",
			    __func__);
		return -1;
	}

	memset (&saddr_ll, 0, sizeof (saddr_ll));
	saddr_ll.sll_family = AF_PACKET;
	saddr_ll.sll_protocol = htons (ETH_P_ALL);
	saddr_ll.sll_pkttype = PACKET_HOST;
	saddr_ll.sll_halen = ETH_ALEN;

	if (bind (sock, (struct sockaddr *)&saddr_ll, sizeof (saddr_ll)) < 0){
		error_warn ("%s: can not bind raw socket, %s",
			    __func__, strerror (errno));
		return -1;
	}

	return sock;
}



int
create_lisp_cmd_socket (void)
{
	int sock;
	struct sockaddr_un saddru;

	memset (&saddru, 0, sizeof (saddru));
	saddru.sun_family = AF_UNIX;
	strncpy (saddru.sun_path, LISP_UNIX_DOMAIN, UNIX_PATH_MAX);

	if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
		err (EXIT_FAILURE, "can not create unix socket");

	if (bind (sock, (struct sockaddr *)&saddru, sizeof (saddru)) < 0)
		error_quit ("can not bind unix socket : %s", strerror (errno));

	return sock;
}




struct lisp lisp;

int
main (int argc, char * argv[])
{
	memset (&lisp, 0, sizeof (lisp));

	openlog (LIXY_LOGNAME, LOG_CONS, LIXY_LOGFACILITY);
	error_enable_syslog ();

	lisp.udp_socket = create_lisp_udp_socket ();
	lisp.ctl_socket = create_lisp_ctl_socket ();
	lisp.cmd_socket = create_lisp_cmd_socket ();
	lisp.raw_socket = create_raw_socket ();
	lisp.tun_socket = tun_alloc (LISP_TUNNAME);

	tun_up (LISP_TUNNAME);

	lisp.eid_tuple = create_list ();
	lisp.loc_tuple = create_list ();
	lisp.mapsrv_tuple = create_list ();

	lisp.cmd_tuple = install_cmd_node ();
	lisp.ctl_message = install_control_message ();

	lisp.rib = create_maptable ();
	
	start_lisp_thread (&(lisp.map_register_t), lisp_map_register_thread);
	start_lisp_thread (&(lisp.map_message_t), lisp_map_message_thread);
	start_lisp_thread (&(lisp.maptable_t), lisp_maptable_thread);
	start_lisp_thread (&(lisp.lisp_dp_t), lisp_dp_thread);
	start_lisp_thread (&(lisp.lisp_dp_tun_t), lisp_dp_tun_thread);
	start_lisp_thread (&(lisp.lisp_raw_packet_t), lisp_raw_packet_thread);

	lisp_op_thread (NULL);

	/* not reached */

	return -1;
}
