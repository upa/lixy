#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "../maptable.h"
#include "../sockaddrmacro.h"

int
main (int argc, char * argv[])
{
	struct maptable * table;
	struct mapnode mn1, mn2, mn3;
	struct mapnode * mnp;
	struct in_addr a1, a2, a3, d1, d2, d3;
	prefix_t p1, p2, p3;

	table = create_maptable ();

	a1.s_addr = inet_addr ("192.168.0.0");
	a2.s_addr = inet_addr ("192.168.1.1");
	a3.s_addr = inet_addr ("10.10.10.10");
	d1.s_addr = inet_addr ("1.1.1.1");
	d2.s_addr = inet_addr ("2.2.2.2");
	d3.s_addr = inet_addr ("3.3.3.3");

	ADDRTOPREFIX (AF_INET, a1, 16, &p1);
	ADDRTOPREFIX (AF_INET, a2, 32, &p2);
	ADDRTOPREFIX (AF_INET, a3, 32, &p3);
	
	ADDRTOLISPSASTORAGE (AF_INET, d1, &(mn1.addr));

	ADDRTOLISPSASTORAGE (AF_INET, d2, &(mn2.addr));

	ADDRTOLISPSASTORAGE (AF_INET, d3, &(mn3.addr));

	update_mapnode (table, &p1, &mn1);
	update_mapnode (table, &p2, &mn2);
	update_mapnode (table, &p3, &mn3);

	mnp = search_mapnode (table, &p1);
	if (mnp != NULL) {
		PRINT_ADDR ("dst is ", mnp->addr);
	} else {
		printf ("NULL!!\n");
	}

	mnp = search_mapnode (table, &p2);
	if (mnp != NULL) {
		PRINT_ADDR ("dst is ", mnp->addr);
	} else {
		printf ("NULL!!\n");
	}

	delete_mapnode (table, &p1);
	update_mapnode (table, &p2, &mn3);

	mnp = search_mapnode (table, &p1);
	if (mnp != NULL) {
		PRINT_ADDR ("dst is ", mnp->addr);
	} else {
		printf ("NULL!!\n");
	}

	mnp = search_mapnode (table, &p2);
	if (mnp != NULL) {
		PRINT_ADDR ("dst is ", mnp->addr);
	} else {
		printf ("NULL!!\n");
	}


	return 0;
}
