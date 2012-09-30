/*
 * list is wrapper of utlist
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <utlist.h>

#include "list.h"


list_t * 
create_list (void)
{
	list_t * li;
	
	li = (list_t *) malloc (sizeof (list_t));
	memset (li, 0, sizeof (li));
	li->head = NULL;
	li->count = 0;

	return li;
}

void
destroy_list (list_t * list)
{
	listnode_t * node;

	LL_FOREACH (list->head, node) {
		free (node->data);
		LL_DELETE (list->head, node);
	}

	return;
}

void
delete_list (list_t * list) 
{
	listnode_t * node;

	LL_FOREACH (list->head, node) {
		LL_DELETE (list->head, node);
	}

	return;
}

void
foreach_list (list_t * list, void (* func)(void * data))
{
	listnode_t * node;
	
	LL_FOREACH (list->head, node) {
		func (node->data);
	}

	return;
}

listnode_t * 
add_listnode (list_t * list, void * data)
{
	listnode_t * node;
	
	node = (listnode_t *) malloc (sizeof (listnode_t));
	memset (node, 0, sizeof (listnode_t));
	
	node->data = data;
	node->prev = NULL;
	node->next = NULL;
	
	LL_APPEND (list->head, node);
	list->count++;

	return node;
}

void *
delete_listnode (list_t * list, void * data)
{
	listnode_t * node;
	
	LL_FOREACH (list->head, node) {
		if (node->data == data) {
			LL_DELETE (list->head, node);
			list->count--;
			return data;
		}
	}

	return NULL;
}

void *
search_listnode (list_t * list, void * d1, int (* cmp)(void * d1, void * d2))
{
	listnode_t * node;

	LL_FOREACH (list->head, node) {
		if (cmp (d1, node->data) == 0) {
			return node->data;
		}
	}

	return NULL;
}
