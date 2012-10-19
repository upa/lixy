#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#ifdef linux
#include <linux/un.h>
#else
#include <sys/proc_info.h>
#endif

#include "common.h"
#include "control.h"
#include "error.h"

void
usage (void)
{
	printf ("\n"
		"lixyctl [element] [value] [action]\n"
		"\n"
		"  -h : print this message\n"
		"  mapserver [v4/v6_prefix] [delete]\n"
		"  locator   [v4/v6_prefix] "
		"[create|delete|priority|weight|m_priority|m_weight] [value]\n"
		"  eid       [eid_name]     [create|delete]\n"
		"  eid       [eid_name]     authkey [keystring]\n"
		"  eid       [eid_name]     interface [interface name]\n"
		"  eid       [eid_name]     prefix [v4/v6_prefix] [delete]\n"
		"  show      ipv4-route     [active|negative|drop|queried|static]\n"
		"  show      ipv6-route     [active|negative|drop|queried|static]\n"
		"  show      eid            [eid name]\n"
		"  show      mapserver      [address]\n"
		"  route     [ipv4|ipv6]    [dst prefix]   [address|delete]\n"
		"\n"
		"\n"
		);
	return;
}

int
create_unix_client_socket (char * domain)
{
        int sock;
        struct sockaddr_un saddru;
        
        memset (&saddru, 0, sizeof (saddru));
        saddru.sun_family = AF_UNIX;
        strncpy (saddru.sun_path, domain, UNIX_PATH_MAX);
        
        if ((sock = socket (AF_UNIX, SOCK_STREAM, 0)) < 0)
                err (EXIT_FAILURE, "can not create unix socket");
        
        if (connect (sock, (struct sockaddr *)&saddru, sizeof (saddru)) != 0)
                err (EXIT_FAILURE, "can not connect to unix socket \"%s\"", domain);
        
        return sock;
}


int
main (int argc, char * argv[])
{
	int n, sock;
	char buf[CONTROL_MSG_BUF_LEN];
	char cmdstr[CONTROL_MSG_BUF_LEN];
	char cmdbuf[CONTROL_MSG_BUF_LEN];
	
	memset (buf, 0, sizeof (buf));
	memset (cmdstr, 0, sizeof (cmdstr));

	if (argc <= 1) {
		usage ();
		return -1;
	} else {
		if (strcmp (argv[1], "-h") == 0) {
			usage ();
			return 0;
		}
	}

	for (n = 1; n < argc; n++){
		if (snprintf (cmdbuf, sizeof (cmdbuf), 
			      "%s %s", cmdstr, argv[n]) > CONTROL_MSG_BUF_LEN)
			error_quit ("command string is too long");
		strncpy (cmdstr, cmdbuf, sizeof (cmdbuf));
	}

	sock = create_unix_client_socket (LISP_UNIX_DOMAIN);
	
	if (write (sock, cmdstr, strlen (cmdstr)) < 0) {
		perror ("write");
		shutdown (sock, SHUT_RDWR);
		close (sock);
	}

	while (read (sock, buf, sizeof (buf))) {
		printf ("%s", buf);
	}

	return 0;
}
