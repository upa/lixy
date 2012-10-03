#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../map.h"
#include "../maptable.h"
#include "../sockaddrmacro.h"

struct lisp lisp;

int
main (int argc, char * argv[])
{

	int len;
	char buf[1024];
	struct eid eid;
	struct locator loc;
	struct in_addr eid_addr;
	

	eid_addr.s_addr = inet_addr ("192.168.0.0");

	memset (&loc, 0, sizeof (loc));
	(EXTRACT_FAMILY (loc.loc_addr)) = AF_INET;
	(EXTRACT_INADDR (loc.loc_addr)).s_addr = inet_addr ("163.221.14.218");

	memset (&eid, 0, sizeof (eid));
	strcpy (eid.authkey, "hoge");
	ADDRTOPREFIX (AF_INET, eid_addr, 24, &(eid.prefix));
	eid.locator = loc;

	len = set_lisp_map_register (buf, sizeof (buf), &eid);
	printf ("register packet length = %d\n", len);

	struct lisp_map_register * reg;
	reg = (struct lisp_map_register *) buf;
	printf ("type = %d\n", reg->type);

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

	printf ("initial regsiter ?\n");
	int n;
	for (n = 0; n < 5; n++) {
		printf ("count %d\n", n);
		if (sendto (sock, buf, len, 0, 
			    (struct sockaddr *)&mapsrv, sizeof (mapsrv)) < 0) {
			perror ("sendto");
		}
		sleep (1);
	}

	printf ("honban\n");
	if (sendto (sock, buf, len, 0, 
		    (struct sockaddr *)&mapsrv, sizeof (mapsrv)) < 0) 
		perror ("sendto");
	

	return 0;
}
