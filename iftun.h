#ifndef _IFTUP_H_
#define _IFTUP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_tun.h>



int tun_alloc (char * dev);
int tun_up (char * dev);


#endif /* _IFTUP_H_ */
