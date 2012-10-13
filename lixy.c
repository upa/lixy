#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>

#include "common.h"
#include "error.h"

int 
create_udp_socket (int port)
{
	int sock;
	int off = 0;
	struct sockaddr_in6 saddr_in6;

	/* IPv4 is supported by Mapped Address */

	if ((sock = socket (AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err (EXIT_FAILURE, "can not create data plane udp socket");
	
	memset (&saddr_in6, 0, sizeof (saddr_in6));
	saddr_in6.sin6_family = AF_INET6;
	saddr_in6.sin6_port = htons (port);

	if (bind (sock, (struct sockaddr *) &saddr_in6, sizeof (saddr_in6)) < 0)
		err (EXIT_FAILURE, "can not bind date plane udp socket");

	if (setsockopt (sock, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof (off)) != 0)
		err (EXIT_FAILURE, "can not set IPV6_V6ONLY off");

	return sock;
}


int 
create_lisp_udp_socket (void)
{
	return create_udp_socket (LISP_DATA_PORT);
}

int 
create_lisp_ctl_socket (void)
{
	return create_udp_socket (LISP_CONTROL_PORT);
}

int 
create_lisp_raw_socket (void)
{
	int sock;

#ifdef linux
	int of = 1;

	if ((sock = socket (AF_UNSPEC, SOCK_RAW, IPPROTO_RAW)) < 0)
		err (EXIT_FAILURE, "can not create raw socket for send packet");
	
	if (setsockopt (sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof (onf)) != 0)
		err (EXIT_FAILURE, "can not set sock opt IPv4 HDRINCL")

	if (setsockopt (sock, IPPROTO_IPV6, IPV6_HDRINCL, &on, sizeof (onf)) != 0)
		err (EXIT_FAILURE, "can not set sock opt IPv6 HDRINCL");
#endif

	return sock;
}



int
create_lisp_cmd_socket (void)
{
	int sock;
	int on = 1;
	struct sockaddr_un saddru;

	memset (&saddru, 0, sizeof (saddru));
	saddru.sun_family = AF_UNIX;
	strncpy (saddru.sun_path, LISP_UNIX_DOMAIN, UNIX_PATH_MAX);

	if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
		err (EXIT_FAILURE, "can not create unix socket");

	if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) != 0)
		err (EXIT_FAILURE, "can not set REUSEADDR to unix socket");

	if (bind (sock, (struct sockaddr *)&saddru, sizeof (saddru)) < 0)
		error_quit ("%s exists\n", LISP_UNIX_DOMAIN);

	return sock;
}




struct lisp lisp;

int
main (int argc, char * argv[])
{
	return 0;
}
