#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maptable.h"
#include "sockaddrmacro.h"

struct maptable *
create_maptable (void)
{
	struct maptable * table;
	
	table = (struct maptable *) malloc (sizeof (struct maptable));
	table->tree = New_Patricia (128);
	table->count = 0;

	return table;
}

void
foreach_maptable (struct maptable * table, 
		  void (* func)(prefix_t * prefix, struct mapnode * node))
{
	patricia_process (table->tree, func);
	return;
}


struct mapnode * 
update_mapnode (struct maptable * table, prefix_t * prefix, struct mapnode * node)
{
	patricia_node_t * pn;
	struct mapnode * mn;

	pn = patricia_lookup (table->tree, prefix);

	if (pn->data != NULL) {
		mn = (struct mapnode *) (pn->data);
		if (mn->state == MAPSTATE_STATIC)
			return NULL;
		free (pn->data);
	}

	pn->data = node;

	return node;
}

struct mapnode *
delete_mapnode (struct maptable * table, prefix_t * prefix)
{
	struct mapnode * mn;
	patricia_node_t * pn;

	pn = patricia_search_best (table->tree, prefix);

	if (pn == NULL)
		return NULL;
		
	mn = pn->data;
	patricia_remove (table->tree, pn);
	table->count--;

	return mn;
}

struct mapnode *
search_mapnode (struct maptable * table, prefix_t * prefix)
{
	patricia_node_t * pn;

	if ((pn = patricia_search_best (table->tree, prefix)) == NULL) 
		return NULL;

	return pn->data;
}

inline void
install_mapnode_queried (struct maptable * table, prefix_t * prefix)
{
	struct mapnode * mn;

	mn = (struct mapnode *) malloc (sizeof (struct mapnode));
	mn->state = MAPSTATE_QUERIED;

	if (update_mapnode (table, prefix, mn) == NULL)
		free (mn);

	return;
}

void
install_mapnode_static (struct maptable * table, prefix_t * prefix, 
			struct sockaddr_storage addr)
{
	struct mapnode * mn;

	mn = (struct mapnode *) malloc (sizeof (struct mapnode));
	memset (mn, 0, sizeof (struct mapnode));

	mn->state = MAPSTATE_STATIC;
	mn->timer = MAPTIMER_DEFAULT;
	mn->addr = addr;

	if (update_mapnode (table, prefix, mn) == NULL)
		free (mn);

	return;
}

int
uninstall_mapnode_static (struct maptable * table, prefix_t * prefix)
{
	struct mapnode * mn;

	if ((mn = search_mapnode (table, prefix)) == NULL) 
		return -1;

	if (mn->state == MAPSTATE_ACTIVE || mn->state == MAPSTATE_QUERIED) 
		return -1;

	delete_mapnode (table, prefix);
	free (mn);

	return 0;
}
