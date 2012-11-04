#ifndef _MAPTABLE_H_
#define _MAPTABLE_H_

#include <sys/types.h>

#include "common.h"
#include "patricia/patricia.h"

struct maptable {
	patricia_tree_t * tree;
	u_int32_t count;
};

struct mapnode {
	int ttl;
	int timer;
	u_int8_t state;
	struct sockaddr_storage addr;
	struct locator locator;
};

#define MAPTALBE_TREE(MT) (MT)->tree
#define MAPTABLE_TREE_HEAD(MT) (MT)->tree->head

#define MAPTTL_DEFAULT		60	/* 1min */
#define MAPTTL_CHECK_INTERVAL	10	/* sec  */

/* Map Cache States */
#define MAPSTATE_ACTIVE		0
#define MAPSTATE_NEGATIVE	1
#define MAPSTATE_DROP		2
#define MAPSTATE_QUERIED	3
#define MAPSTATE_STATIC		4


#define MAPSTATE_STRING_INIT()			\
	{					\
		"active",			\
		"negative",			\
		"drop",				\
		"queried",			\
		"static"			\
	}					\

/* Prototypes */

struct maptable * create_maptable (void);
void foreach_maptable (struct maptable * table, 
		       void (*func)(prefix_t * prefix, struct mapnode * node));

struct mapnode * update_mapnode (struct maptable * table, prefix_t * prefix, 
				 struct mapnode * node);
struct mapnode * delete_mapnode (struct maptable * table, prefix_t * prefix);
struct mapnode * search_mapnode (struct maptable * table, prefix_t * prefix);

void install_mapnode_queried (struct maptable * table, prefix_t * prefix);


void install_mapnode_static (struct maptable * table, prefix_t * prefix, 
			     struct sockaddr_storage addr);

/* return value : 0 is sucess, -1 is faield */
int uninstall_mapnode_static (struct maptable * table, prefix_t * prefix);

/* process maptable thread. it decrease ttl and release cache */
void * lisp_maptable_thread (void * param);


/* Macros */

#define _ADDRTOPREFIX(af, addr, len, prefix)	\
	do {					\
		(prefix)->family = AF_INET##af;	\
		(prefix)->bitlen = len;		\
		(prefix)->ref_count = 0;	\
		(prefix)->add.sin##af = *((struct in##af##_addr *)&(addr)); \
	} while (0)

#define ADDRTOPREFIX(af, sin, len, prefix)		\
	switch (af) {					\
	case AF_INET :					\
		_ADDRTOPREFIX (, sin, len, prefix);	\
		break;					\
	case AF_INET6 :					\
		_ADDRTOPREFIX (6, sin, len, prefix);	\
                break;					\
	default :							\
		error_warn ("%s: invalid AF \"%d\"in ADDRTOPREFIX",	\
			    __func__, af);				\
        }



#define _ADDRTOSASTORAGE(af, sinaddr, sas)				\
	do {								\
		(sas)->ss_len = sizeof (struct sockaddr_in##af);	\
		(sas)->ss_family = AF_INET##af;				\
		struct sockaddr_in##af * sa;				\
		sa = (struct sockaddr_in##af *) sas;			\
		sa->sin##af##_port = htons (LISP_DATA_PORT);		\
		sa->sin##af##_addr = *((struct in##af##_addr *)&sinaddr); \
	} while (0)
	
#define ADDRTOLISPSASTORAGE(af, sin, ss)	\
	switch (af) {				\
	case AF_INET :				\
		_ADDRTOSASTORAGE(, sin, ss);	\
		break;				\
	case AF_INET6:				\
		_ADDRTOSASTORAGE(6, sin, ss);	\
		break;				\
	}					\
		

	

#endif /* _MAPTABLE_H_ */
