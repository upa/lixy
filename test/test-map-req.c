#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <arpa/inet.h>

#include "../map.h"
#include "../maptable.h"
#include "../sockaddrmacro.h"
#include "../list/list.h"

struct lisp lisp;

int
main (int argc, char * argv[])
{
	int len;
	char buf[1024];
	prefix_t prefix;
	struct in_addr a;
	struct locator loc;
	
	memset (&lisp, 0, sizeof (lisp));
	memset (&loc, 0, sizeof (loc));
	EXTRACT_FAMILY (loc.loc_addr) = AF_INET;
	EXTRACT_INADDR (loc.loc_addr).s_addr = inet_addr ("133.11.123.222");

	lisp.loc_tuple = create_list ();
	append_listnode (lisp.loc_tuple, &loc);

	a.s_addr = inet_addr ("192.168.0.0");
	ADDRTOPREFIX (AF_INET, a, 24, &prefix);
	
	len = set_lisp_map_request (buf, sizeof (buf), &prefix);
	if (len < 0) {
		printf ("set_lisp_map_request failed : no locator address\n");
		return -1;
	}

	int sock;
	struct addrinfo hints, * res;
	struct sockaddr_in mapsrv;
	memset (&mapsrv, 0, sizeof (mapsrv));
	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	int err;
	if ((err = getaddrinfo ("203.178.139.68", NULL, &hints, &res)) != 0) {
		printf ("getaddrinfo : %s\n", gai_strerror (err));
	}

	sock = socket (res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock < 0) {
		perror ("socket");
	}

	memcpy (&mapsrv, res->ai_addr, res->ai_addrlen);
	EXTRACT_PORT (mapsrv) = htons (LISP_CONTROL_PORT);

	freeaddrinfo (res);
	printf ("socket is %d\n", sock);

	if (sendto (sock, buf, len, 0,
		    (struct sockaddr *)&mapsrv, sizeof (mapsrv)) < 0) {
		perror ("sendto");
	}

	return 0;
}
