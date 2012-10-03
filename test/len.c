#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "../lisp.h"

int
main (int argc, char * argv[])
{
	printf ("LISP ENCAP %ld\n", sizeof (struct lisp_control));
	printf ("IP %ld\n", sizeof (struct ip));
	printf ("UDP %ld\n", sizeof (struct udphdr));
	printf ("MAPREQ %ld\n", sizeof (struct lisp_map_request));
	printf ("Source EID AFI%ld\n", sizeof (u_int16_t));
	printf ("ITR EID AFI %ld\n", sizeof (u_int16_t));
	printf ("ITR ADDRESS %ld\n", sizeof (struct in_addr));
	printf ("MAP REC %ld\n", sizeof (struct lisp_map_request_record));
	printf ("EID PREFIX %ld\n", sizeof (struct in_addr));

	return 0;
}
