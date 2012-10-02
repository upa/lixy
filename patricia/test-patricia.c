#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "patricia.h"

void 
process_func (prefix_t * prefix, void * data)
{
	if (data == NULL) {
		printf ("data is NULL\n");
	} else {
		char * p = data;
		printf ("data is \"%s\"\n", p);
	}
	printf ("%s/%d\n", inet_ntoa (prefix->add.sin), prefix->bitlen);
	return;
}


int
main (int argc, char * argv[])
{
	patricia_tree_t * v4table;
	patricia_node_t * node;
	prefix_t * p1, * p2, * p3;
	char buf[] = "data!!";

	v4table = New_Patricia (128);
	
	p1 = ascii2prefix (AF_INET, "8.8.8.8/32");
	p2 = ascii2prefix (AF_INET, "192.168.0.0/16");
	p3 = ascii2prefix (AF_INET, "192.168.1.0/24");
	p3 = ascii2prefix (AF_INET, "192.168.1.0/24");

	node = patricia_lookup (v4table, p1);
	if (node->data == NULL) {
		printf ("node is NULL\n");
		node->user1 = buf;
	}
	patricia_lookup (v4table, p2);
	patricia_lookup (v4table, p3);

	patricia_process (v4table, process_func);


	printf ("\nlookup!\n");

	prefix_t *pp;
	patricia_node_t * pn;

#define PREFIX "8.8.8.8/24"
	pp = ascii2prefix (AF_INET, PREFIX);
	pn = patricia_search_best (v4table, pp);
	if (pn == NULL) {
		printf ("%s does not exists\n", PREFIX);
	} else {
		process_func (pn->prefix, NULL);
	}

#define PREFIX "192.168.3.0/24"
	pp = ascii2prefix (AF_INET, PREFIX);
	pn = patricia_search_best (v4table, pp);
	if (pn == NULL) {
		printf ("%s does not exists\n", PREFIX);
	} else {
		printf ("%s -> ", PREFIX);
		process_func (pn->prefix, NULL);
	}

#define PREFIX "192.168.1.1/32"
	pp = ascii2prefix (AF_INET, PREFIX);
	pn = patricia_search_best (v4table, pp);
	if (pn == NULL) {
		printf ("%s does not exists\n", PREFIX);
	} else {
		printf ("%s -> ", PREFIX);
		process_func (pn->prefix, NULL);
	}


#define PREFIX "10.10.10.10/32"
	pp = ascii2prefix (AF_INET, PREFIX);
	pn = patricia_search_best (v4table, pp);
	if (pn == NULL) {
		printf ("%s does not exists\n", PREFIX);
	} else {
		printf ("%s -> ", PREFIX);
		process_func (pn->prefix, NULL);
	}




	return 0;
}
