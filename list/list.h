/*
 * list is wrapper of utlist
 */

#ifndef _LIST_H_
#define _LIST_H_

#include <sys/types.h>

typedef struct _listnode_t {
	void * data;
	struct _listnode_t * prev;
	struct _listnode_t * next;
} listnode_t;

typedef struct _list_t {
	listnode_t * head;
	u_int32_t count;
} list_t;


/* Prototypes */
list_t * create_list (void);
void destroy_list (list_t * list);		/* free user date */
void delete_list (list_t * list);		/* Does not free user date */
void foreach_list (list_t * list, void (* func)(void * data));

listnode_t * add_listnode (list_t * list, void * data);
void * delete_listnode (list_t * list, void * data);	/* delete node that has data ptr */
void * search_listnode (list_t * list, void * d1, int (* cmp)(void * d1, void * d2));


#define MYLIST_FOREACH(li, linode) LL_FOREACH (li->head, linode)

#define MYLIST_COUNT(li) li->count

#endif /* _LIST_H_ */
