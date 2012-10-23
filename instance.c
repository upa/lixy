#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#ifdef linux
#include <linux/if_packet.h>
#endif

#include "map.h"
#include "maptable.h"
#include "instance.h"
#include "sockaddrmacro.h"

#include "error.h"

int start_eid_forwarding_thread (struct eid * eid);
void * eid_forwarding_thread (void * param);

		
#define IS_EID_THREAD_RUNNING(eid) ((eid)->t_flag > 0)

#define STOP_EID_THREAD(eid)					\
	do {							\
		if (IS_EID_THREAD_RUNNING ((eid))) {		\
			if (pthread_cancel ((eid)->tid) == 0)	\
				(eid)->t_flag = -1;		\
			else					\
				error_warn ("faild to stop "	\
					    "eid \"%s\"",	\
					    eid->name);		\
		}						\
	} while (0)


#define START_EID_THREAD(eid)						\
	do {								\
		if (!IS_EID_THREAD_RUNNING ((eid))) {			\
			if (start_eid_forwarding_thread ((eid)) > 0)	\
				(eid)->t_flag = 1;			\
			else						\
				error_warn ("%s: start thread failed",	\
					    __func__);			\
		} 							\
	} while (0)


int 
create_raw_socket (char * ifname)
{
	int sock;
#ifdef linux
	unsigned int ifindex;
	struct sockaddr_ll saddr_ll;
	
	if ((ifindex = if_nametoindex (ifname)) < 1) {
		error_warn ("%s: interface \"%s\" is does not exits", __func__, ifname);
		return -1;
	}

	if ((sock = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		error_warn ("%s: can not create raw socket for \"%s\"", __func__, ifname);
		return -1;
	}
	
	memset (&saddr_ll, 0, sizeof (saddr_ll));
	saddr_ll.sll_family = AF_PACKET;
	saddr_ll.sll_protocol = htons (ETH_P_ALL);
	saddr_ll.sll_ifindex = ifindex;

	if (bind (sock, (struct sockaddr *)&saddr_ll, sizeof (saddr_ll)) < 0){
		error_warn ("%s: can not bind raw socket to \"%s\" : %s", 
			    __func__, ifname, strerror (errno));
		return -1;
	}
#endif

	return sock;
}

int
sendraw4 (int fd, void * packet)
{
        struct sockaddr_in saddr_in;
        struct ip * ip = (struct ip *) packet;

        saddr_in.sin_addr = ip->ip_dst;
        saddr_in.sin_family = AF_INET;
        
        return sendto (fd, packet, ntohs (ip->ip_len), 0, 
                       (struct sockaddr *) &saddr_in, sizeof (saddr_in));
}

int
sendraw6 (int fd, void * packet)
{
        struct sockaddr_in6 saddr_in6;
        struct ip6_hdr * ip6 = (struct ip6_hdr *) packet;

        saddr_in6.sin6_addr = ip6->ip6_dst;
        saddr_in6.sin6_family = AF_INET6;
        
        return sendto (fd, packet, 
		       ntohs (ip6->ip6_plen) + sizeof (struct ip6_hdr),
		       0, (struct sockaddr *) &saddr_in6, 
		       sizeof (saddr_in6));
}

struct eid * 
create_eid_instance (char * eidname)
{
	struct eid * eid;
	
	eid = (struct eid *) malloc (sizeof (struct eid));
	memset (eid , 0, sizeof (struct eid));
	strncpy (eid->name, eidname, LISP_EID_NAME_LEN);
	eid->t_flag = -1;
	eid->prefix_tuple = create_list ();

	return eid;
}

struct eid * 
delete_eid_instance (struct eid * eid)
{
	STOP_EID_THREAD (eid);
	if (eid->raw_socket > 0) 
		close (eid->raw_socket);
	
	return eid;
}

int
set_eid_iface (struct eid * eid, char * ifname)
{
	if (IS_EID_THREAD_RUNNING (eid)) 
		STOP_EID_THREAD (eid);

	close (eid->raw_socket);
	eid->raw_socket = create_raw_socket (ifname);
	if (eid->raw_socket < 0)
		return -1;
	strncpy (eid->ifname, ifname, IFNAMSIZ);		
	START_EID_THREAD (eid);

	return 0;
}

int
set_eid_authkey (struct eid * eid, char * authkey) 
{
	strncpy (eid->authkey, authkey, LISP_MAX_KEYLEN);
	return 0;
}

int
set_eid_prefix (struct eid * eid, prefix_t * prefix)
{
	append_listnode (eid->prefix_tuple, prefix);
	return 0;
}

int
unset_eid_iface (struct eid * eid)
{
	STOP_EID_THREAD (eid);
	close (eid->raw_socket);
	eid->raw_socket = -1;
	memset (eid->ifname, 0, IFNAMSIZ);
	eid->ifname[0] = '\0';

	return 0;
}

int
unset_eid_authkey (struct eid * eid)
{
	memset (eid->authkey, 0, LISP_MAX_KEYLEN);
	eid->authkey[0] = '\0';

	return 0;
}

int
prefix_cmp (void * d1, void * d2)
{
	prefix_t * p1, * p2;

	p1 = (prefix_t *) d1;
	p2 = (prefix_t *) d2;
	
	if (p1->family != p2->family) 
		return -1;
	
	switch (p1->family) {
	case AF_INET :
		if (COMPARE_SADDR_IN (p1->add.sin, p2->add.sin))
			return 0;
		else
			return -1;
		break;
	case AF_INET6 :
		if (COMPARE_SADDR_IN6 (p1->add.sin6, p2->add.sin6))
			return 0;
		else
			return -1;
		break;
	}

	return -1;
}

int
unset_eid_prefix (struct eid * eid, prefix_t * prefix)
{
	prefix_t * pp;

	pp = search_listnode (eid->prefix_tuple, prefix, prefix_cmp);
	if (pp == NULL) {
		error_warn ("%s: unmatch delete prefix", __func__);
		return -1;
	}
	
	delete_listnode (eid->prefix_tuple, pp);
	free (pp);

	return 0;
}


int 
start_eid_forwarding_thread (struct eid * eid)
{
	pthread_attr_t attr;

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	
	/* check interface */
	if (if_nametoindex (eid->ifname) < 1) {
		error_warn ("%s: EID \"%s\", interface \"%s\" does not exists",
			    eid->name, eid->ifname);
		return -1;
	}

	/* check socket */
	if (eid->raw_socket < 0) {
		error_warn ("%s: EID \"%s\", interface \"%s\" "
			    "invalid raw socket",
			    eid->name, eid->ifname);
		return -1;
	}

	error_warn ("%s: start forwarding thread for EID \"%s\"", 
		    __func__, eid->name);
	pthread_create (&(eid->tid), &attr, eid_forwarding_thread, eid);

	return 1;
}

void *
eid_forwarding_thread (void * param)
{
	char buf[LISP_EID_DP_BUF_LEN];
	struct eid * eid;
	struct msghdr mhdr;
	struct iovec iov[2];
	struct lisp_hdr lhdr;
	struct ether_header * ehdr;
	struct ip * ip;
	struct ip6_hdr * ip6;
	struct mapnode * mn;
	prefix_t dst_prefix;
	
	eid = (struct eid *) param;

	/* create LISP Encapsulation header */
	memset (&lhdr, 0, sizeof (lhdr));
	lhdr.N_flag = 1;
	lhdr.L_flag = 1;
	lhdr.lhdr_loc_status = htonl (1);

	mhdr.msg_iov = iov;
	mhdr.msg_iovlen = 2;
	mhdr.msg_controllen = 0;
	iov[0].iov_base = &lhdr;
	iov[0].iov_len = sizeof (lhdr);
	iov[1].iov_base = buf + sizeof (struct ether_header);

	while (1) {
		if (recv (eid->raw_socket, buf, sizeof (buf), 0) < 0) {
			error_warn ("EID \"%d\" recv failed", eid->name);
			break;
		}

		ehdr = (struct ether_header *) buf;		
		switch (ntohs (ehdr->ether_type)) {
		case ETH_P_IP :
			ip = (struct ip *)(buf + sizeof (struct ether_header));
			ADDRTOPREFIX (AF_INET, ip->ip_dst, 32, &dst_prefix);
			mn = search_mapnode (lisp.rib, &dst_prefix);
			break;

		case ETH_P_IPV6 :
			ip6 = (struct ip6_hdr *)
				(buf + sizeof (struct ether_header));
			ADDRTOPREFIX (AF_INET6, ip6->ip6_dst, 
				      128, &dst_prefix);
			mn = search_mapnode (lisp.rib, &dst_prefix);
			break;

		default :
			continue;
		}

		if (mn == NULL) {
			/* Send Map Request */
			prefix_t * i_prefix;
			i_prefix = (prefix_t *) malloc (sizeof (prefix_t));
			* i_prefix = dst_prefix;
			install_mapnode_queried (lisp.rib, i_prefix);
			send_map_request (&dst_prefix);
			continue;
		} 

		if (mn->state == MAPSTATE_NEGATIVE || 
		    mn->state == MAPSTATE_QUERIED) {
			/* native forwarding */
			switch (ntohs (ehdr->ether_type)) {
			case AF_INET : 
				sendraw4 (lisp.raw4_socket, ip);
				break;
			case AF_INET6 :
				sendraw6 (lisp.raw6_socket, ip);
				break;
			}
		} else if (mn->state == MAPSTATE_ACTIVE ||
			   mn->state == MAPSTATE_STATIC) {
			/* LISP Encapsulated forwarding to LISP SITE */
			mhdr.msg_name = &(mn->addr);
			mhdr.msg_namelen = EXTRACT_SALEN(mn->addr);
			sendmsg (lisp.udp_socket, &mhdr, 0);
		}
		/* MAPSTATE_DROP is processed */
	}

	return NULL;
}



/* Mail LISP Thread Instance */

int
set_lisp_mapserver (struct sockaddr_storage mapsrv)
{
	if (EXTRACT_FAMILY (mapsrv) != AF_INET &&
	    EXTRACT_FAMILY (mapsrv) != AF_INET6) {
		error_warn ("%s: invalid map server address", __func__);
		return -1;
	}
	
	lisp.mapsrvaddr = mapsrv;

	return 0;
}

int
unset_lisp_mapserver (void)
{
	memset (&(lisp.mapsrvaddr), 0, sizeof (lisp.mapsrvaddr));

	return 0;
}

int
set_lisp_locator (struct locator loc)
{
	struct locator * nloc;

	nloc = (struct locator *) malloc (sizeof (struct locator));
	memset (nloc, 0, sizeof (struct locator));
	*nloc = loc;

	append_listnode (lisp.loc_tuple, nloc);

	return 0;
}

int
unset_lisp_locator (struct sockaddr_storage loc_addr)
{
	listnode_t * li;
	struct locator * loc;

	MYLIST_FOREACH (lisp.loc_tuple, li) {
		loc = (struct locator *) (li->data);
		if (COMPARE_SOCKADDR (&(loc->loc_addr), &(loc_addr))) {
			delete_listnode (lisp.loc_tuple, loc);
			return 1;
		}
	}

	return -1;
}



void
start_lisp_thread (pthread_t * tid, void * (* thread) (void * param))
{
	pthread_attr_t attr;

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
	pthread_create (tid, &attr, thread, NULL);

	return;
}

void * 
lisp_map_register_thread (void * param)
{
	listnode_t * ln;
	struct eid * eid;

	while (1) {
		MYLIST_FOREACH (lisp.eid_tuple, ln) {
			/* if Map server is not set, do not process */
			if (EXTRACT_FAMILY (lisp.mapsrvaddr) == 0) 
				continue;

			eid = (struct eid *) (ln->data);
			send_map_register (eid);
		}
		sleep (LISP_MAP_REGISTER_INTERVAL);
	}

	return NULL;
}


void * 
lisp_map_message_thread (void * param)
{
	int len;
	char buf[LISP_MSG_BUF_LEN];

	while (1) {
		len = recv (lisp.ctl_socket, buf, sizeof (buf), 0);
		if (len < 0) {
			error_warn ("%s: recv failed", __func__);
			continue;
		}
		if (len < sizeof (struct ip) + sizeof (struct lisp_control)) {
			error_warn ("%s: recieved lisp control message "
				    "is too short",
				    __func__);
			continue;
		}
		error_warn ("%s: recieved length is %d", __func__, len);
		process_lisp_map_message (buf);
	}

	return NULL;
}

void * 
lisp_dp_thread (void * param)
{
	/* 
	 * reciev encapsulated LISP DP Packet
	 * and decapsulate it, and send to IP_HDRINCL raw socket
	 */
	
	int len;
	char buf[LISP_EID_DP_BUF_LEN];
	struct lisp_hdr * hdr;
	struct ip * ip;

	hdr = (struct lisp_hdr *) buf;
	ip = (struct ip *) (hdr + 1);
	
	while (1) {
		len = recv (lisp.udp_socket, buf, sizeof (buf), 0);
		if (len < 0) {
			error_warn ("%s: recv failed", __func__);
			continue;
		}

		/* processing all of flags is not supported tehepero */

		switch (ip->ip_v) {
		case 4 :
			sendraw4 (lisp.raw4_socket, ip);
			break;
		case 6 :
			sendraw6 (lisp.raw6_socket, ip);
			break;
		default :
			error_warn ("%s: invalid ip version %d", 
				    __func__, ip->ip_v);
			break;
		}
	}

	return NULL;
}
