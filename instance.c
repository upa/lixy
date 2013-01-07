#include <stdlib.h>
#include <unistd.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <net/if.h>

#ifdef linux
#include <linux/if_packet.h>
#endif

#include "map.h"
#include "maptable.h"
#include "instance.h"
#include "sockaddrmacro.h"

#include "error.h"

#if 0
int 
create_raw_socket (char * ifname)
{
	int sock;
#ifdef linux
	unsigned int ifindex;
	struct sockaddr_ll saddr_ll;
	
	if ((ifindex = if_nametoindex (ifname)) < 1) {
		error_warn ("%s: interface \"%s\" is does not exits", 
			    __func__, ifname);
		return -1;
	}

	if ((sock = socket (AF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
		error_warn ("%s: can not create raw socket for \"%s\"", 
			    __func__, ifname);
		return -1;
	}
	
	memset (&saddr_ll, 0, sizeof (saddr_ll));
	saddr_ll.sll_family = AF_PACKET;
	saddr_ll.sll_protocol = htons (ETH_P_ALL);
	saddr_ll.sll_ifindex = ifindex;
	saddr_ll.sll_pkttype = PACKET_HOST;
	saddr_ll.sll_halen = ETH_ALEN;

	if (bind (sock, (struct sockaddr *)&saddr_ll, sizeof (saddr_ll)) < 0){
		error_warn ("%s: can not bind raw socket to \"%s\" : %s", 
			    __func__, ifname, strerror (errno));
		return -1;
	}
#endif

	return sock;
}
#endif


int
sendraw4 (int fd, void * packet)
{
        static struct sockaddr_in saddr_in;
        struct ip * ip = (struct ip *) packet;

        saddr_in.sin_addr = ip->ip_dst;
        saddr_in.sin_family = AF_INET;

        return sendto (fd, packet, ntohs (ip->ip_len), 0, 
                       (struct sockaddr *) &saddr_in, sizeof (saddr_in));
}

int
sendraw6 (int fd, void * packet)
{
        static struct sockaddr_in6 saddr_in6;
        struct ip6_hdr * ip6 = (struct ip6_hdr *) packet;

        saddr_in6.sin6_addr = ip6->ip6_dst;
        saddr_in6.sin6_family = AF_INET6;
        
        return sendto (fd, packet, 
		       ntohs (ip6->ip6_plen) + sizeof (struct ip6_hdr),
		       0, (struct sockaddr *) &saddr_in6, 
		       sizeof (saddr_in6));
}

int sendraw (int fd, void * packet)
{
	struct ip * ip = (struct ip *) packet;
	if (ip->ip_v == 4) 
		return sendraw4 (fd, packet);
	else if (ip->ip_v == 6)
		return sendraw6 (fd, packet);
	else 
		error_warn ("%s: invalid ip version %d", __func__, ip->ip_v);

	return -1;
}

struct eid * 
create_eid_instance (char * eidname)
{
	struct eid * eid;

	eid = (struct eid *) malloc (sizeof (struct eid));
	memset (eid , 0, sizeof (struct eid));
	strncpy (eid->name, eidname, LISP_EID_NAME_LEN);
	eid->prefix_tuple = create_list ();

	return eid;
}

struct eid * 
delete_eid_instance (struct eid * eid)
{
	return eid;
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



/* Mail LISP Thread Instance */

int
set_lisp_mapserver (struct sockaddr_storage mapsrv)
{
	struct sockaddr_storage * mapsrvaddr;

	mapsrvaddr = (struct sockaddr_storage *) 
		malloc (sizeof (struct sockaddr_storage));

	if (EXTRACT_FAMILY (mapsrv) != AF_INET &&
	    EXTRACT_FAMILY (mapsrv) != AF_INET6) {
		error_warn ("%s: invalid map server address", __func__);
		return -1;
	}
	
	*mapsrvaddr = mapsrv;

	append_listnode (lisp.mapsrv_tuple, mapsrvaddr);

	return 0;
}

int
unset_lisp_mapserver (struct sockaddr_storage mapsrv)
{
	listnode_t * ln;
	struct sockaddr_storage * mapsrvaddr;

	MYLIST_FOREACH (lisp.mapsrv_tuple, ln) {
		mapsrvaddr = (struct sockaddr_storage *) (ln->data);
		if (memcmp (&mapsrv, mapsrvaddr, 
			    sizeof (struct sockaddr_storage)) == 0) {
			delete_listnode (lisp.mapsrv_tuple, mapsrvaddr);
			return 0;
		}
	}

	return -1;
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
	struct eid * eid;
	listnode_t * ln;

	while (1) {
		MYLIST_FOREACH (lisp.eid_tuple, ln) {
			/* if Map server is not set, do not process */
			if (MYLIST_COUNT (lisp.mapsrv_tuple) > 0) {
				eid = (struct eid *) (ln->data);
				send_map_register (eid);
			}
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

		if (write (lisp.tun_socket, ip, 
			    len - sizeof (struct lisp_hdr)) < 0)
			error_warn ("%s: sendto IP(v4/v6) paceket "
				    "faield, %s", __func__,
				    strerror (errno));
	}

	return NULL;
}

void *
lisp_dp_tun_thread (void * param)
{
	/*
	  this thread recieves packets from tun interface,
	  and encapsulate these packets and xmit to the locator.
	 */

	int len;
	char buf[LISP_EID_DP_BUF_LEN];
	struct msghdr mhdr;
	struct iovec iov[2];
	struct lisp_hdr lhdr;
	struct ip * ip;
	struct ip6_hdr * ip6;
	struct mapnode * mn;
	prefix_t dstprefix;

	memset (&lhdr, 0, sizeof (lhdr));
	lhdr.N_flag = 1;
	lhdr.lhdr_loc_status = htonl (1);

	mhdr.msg_iov = iov;
	mhdr.msg_iovlen = 2;
	mhdr.msg_controllen = 0;
	iov[0].iov_base = &lhdr;
	iov[0].iov_len = sizeof (lhdr);
	iov[1].iov_base = buf;

	ip = (struct ip *) buf;
	ip6 = (struct ip6_hdr *) buf;

	while (1) {
		if ((len = read (lisp.tun_socket, buf, sizeof (buf))) < 0) {
			error_warn ("%s: recv from tun socket failed, %s",
				    __func__, strerror (errno));
			continue;
		}

		switch (ip->ip_v) {
		case 4 :
			ADDRTOPREFIX (AF_INET, ip->ip_dst, 32, &dstprefix);
			break;
		case 6:
			ADDRTOPREFIX (AF_INET6, ip6->ip6_dst, 128, &dstprefix);
			break;
		default :
			continue;
		}
		
		if ((mn = search_mapnode (lisp.rib, &dstprefix)) == NULL ||
		    mn->state == MAPSTATE_NEGATIVE ||
		    mn->state == MAPSTATE_QUERIED) {
			if (sendraw (lisp.tun_socket, buf) < 0)
				error_warn ("%s: send to tun failed, %s",
					    __func__, strerror (errno));
		} 
		else if (mn->state == MAPSTATE_ACTIVE ||
			   mn->state == MAPSTATE_STATIC) {
			iov[1].iov_len = len + sizeof (struct lisp_hdr);
			mhdr.msg_name = &(mn->addr);
			mhdr.msg_namelen = EXTRACT_SALEN (mn->addr);
			sendmsg (lisp.udp_socket, &mhdr, 0);
		}
	}

	return NULL;
}

void *
lisp_raw_packet_thread (void * param)
{
	/*
	  this thread watches all packet to localhost,
	  and ask map server who is the locator of 
	  destination prefix of the packets
	 */

	char buf[LISP_EID_DP_BUF_LEN];
	struct ether_header * ehdr;
	struct ip * ip;
	struct ip6_hdr * ip6;
	struct mapnode * mn;
	prefix_t dstprefix;

	ip = (struct ip *)(buf + sizeof (struct ether_header));
	ip6 = (struct ip6_hdr *)(buf + sizeof (struct ether_header));

	while (1) {
		if (recv (lisp.raw_socket, buf, sizeof (buf), 0) < 0) {
			error_warn ("%s: raw socket recv failed, %s",
				    __func__, strerror (errno));
			continue;
		}

		ehdr = (struct ether_header *) buf;

		switch (ntohs (ehdr->ether_type)) {
		case ETH_P_IP :
			if (ip->ip_ttl-- < 1)
				continue;
			if (is_route_scope_link (AF_INET, &(ip->ip_dst)))
				continue;
			ADDRTOPREFIX (AF_INET, ip->ip_dst, 32, &dstprefix);
			break;
		case ETH_P_IPV6 :
			if (ip6->ip6_hlim-- < 1)
				continue;
			if (is_route_scope_link (AF_INET6, &(ip6->ip6_dst)))
				continue;
			ADDRTOPREFIX (AF_INET6, ip6->ip6_dst, 128, &dstprefix);
			break;
		default :
			continue;
		}

		/* Route does not exists on local scope and maptable,
		   ask map server */
		if ((mn = search_mapnode (lisp.rib, &dstprefix)) == NULL) {
			prefix_t * i_prefix;
			i_prefix = (prefix_t *) malloc (sizeof (prefix_t));
			*i_prefix = dstprefix;
			install_mapnode_queried (lisp.rib, i_prefix);
			send_map_request (&dstprefix);
		}
	}

	return NULL;
}


int
is_route_scope_link (int af, void * addr)
{
	int n, fd, prefix_len, addrlen, rtlen;
	int gateway_exists = 0;
	int scope_link = 0;
	char buf[1024];
	struct rtattr * rta;
	struct sockaddr_nl sa_nl;
	struct nlmsghdr * nh;
	struct rtmsg * rt;
	struct rtattr * ra;

	struct {
		struct nlmsghdr nh;
		struct rtmsg rt;
		char buf[1024];
	} req;

	union add {
		struct in_addr sin;
		struct in6_addr sin6;
	} gwy, dst;


	if (af != AF_INET && af != AF_INET6) 
		return -1;

	prefix_len = (af == AF_INET) ? 32 : 128;
	addrlen = (af == AF_INET) ? 
		sizeof (struct in_addr) : sizeof (struct in6_addr);

	/* construct NetLink Mesage structure */
	memset (&req, 0, sizeof (req));
	req.nh.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
	req.nh.nlmsg_type = RTM_GETROUTE;
	req.nh.nlmsg_flags = NLM_F_REQUEST;
	req.nh.nlmsg_seq = 1;
	req.rt.rtm_family = af;
	req.rt.rtm_dst_len = prefix_len;
	req.rt.rtm_table = RT_TABLE_MAIN;
	req.rt.rtm_scope = RT_SCOPE_LINK;
	req.rt.rtm_type = RTN_UNICAST;
	
	rta = (struct rtattr *)
		(((char *)&req) + NLMSG_ALIGN (req.nh.nlmsg_len));
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH (addrlen);
	memcpy (RTA_DATA (rta), addr, addrlen);
	
	req.nh.nlmsg_len = 
		NLMSG_ALIGN (req.nh.nlmsg_len) + rta->rta_len;

	if ((fd = socket (AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
		fprintf (stderr, "%s: can not create NETLINK socket, %s",
			 __func__, strerror (errno));
		return -1;
	}

	memset (&sa_nl, 0, sizeof (sa_nl));
	sa_nl.nl_family = AF_NETLINK;

	if (sendto (fd, &req, sizeof (req), 0, 
		    (struct sockaddr *)&sa_nl, sizeof (sa_nl)) < 0) {
		fprintf (stderr, "%s: can not send NETLINK message, %s",
			 __func__, strerror (errno));
		return -1;
	}
		
	memset (buf, 0, sizeof (buf));
	
	if ((n = read (fd, buf, sizeof (buf))) < 0) {
		fprintf (stderr, "%s: read(2) failed, %s",
			 __func__, strerror (errno));
		return -1;
	}

	nh = (struct nlmsghdr *) buf;
	
	/* check return value from nl socket */
	for (; 
	     nh->nlmsg_type != NLMSG_DONE && NLMSG_OK (nh, n);
	     nh = NLMSG_NEXT (nh, n)) {
		rt = (struct rtmsg *) NLMSG_DATA (nh);
		ra = (struct rtattr *) RTM_RTA (rt);
		rtlen = RTM_PAYLOAD (nh);

		for(; RTA_OK (ra, rtlen); ra = RTA_NEXT (ra, rtlen)) {
			switch (ra->rta_type) {
			case RTA_GATEWAY :
				gateway_exists = 1;
				memcpy (&gwy, RTA_DATA (ra), addrlen);
				break;

			case RTA_DST :
				memcpy (&dst, RTA_DATA (ra), addrlen);
				break;
			}
			if (rt->rtm_scope == RT_SCOPE_LINK)
				scope_link = 1;
		}
	}
	
	close (fd);

	/* in IPv4, kernel does not returned "RTA_GATEWAY"
	   when it is link scope destination
	 */
	if (!gateway_exists || scope_link) 
		return 1;
		      
	/* in IPv6, kernel return gateway address (RTA_GATEWAY) that
	   is same as destination address (RTA_DST)
	 */
	if (af == AF_INET6) {
		if (memcmp (&gwy, &dst, addrlen) == 0)
			return 1;
	}

	return 0;
}

int
op_route_to_dev (int op, int af, void * addr, int prefixlen, char * tunifname)
{
	int fd;
	int addrlen;
	u_int32_t tunifindex;

	struct rtattr * rta;
	struct sockaddr_nl sa_nl;

	struct {
		struct nlmsghdr nh;
		struct rtmsg rt;
		char buf[1024];
	} req;

	addrlen = (af == AF_INET) ? 
		sizeof (struct in_addr) : sizeof (struct in6_addr);

	if ((tunifindex = if_nametoindex (tunifname)) < 1) 
		return -1;

	memset (&req, 0, sizeof (req));
	req.nh.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
	req.nh.nlmsg_type = op;
	req.rt.rtm_family = af;
	req.rt.rtm_table = RT_TABLE_MAIN;
	req.rt.rtm_dst_len = prefixlen;
	
	switch (op) {
	case RTM_NEWROUTE :
		req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
		req.rt.rtm_protocol = RTPROT_BOOT;
		req.rt.rtm_scope = RT_SCOPE_UNIVERSE;
		req.rt.rtm_type = RTN_UNICAST;
		break;
	case RTM_DELROUTE :
		req.nh.nlmsg_flags = NLM_F_REQUEST;
		req.rt.rtm_scope = RT_SCOPE_NOWHERE;
		break;
	default :
		return -1;
	}


	/* set destination prefix */
	rta = (struct rtattr *) 
		((char *)&req + NLMSG_ALIGN (req.nh.nlmsg_len));
	rta->rta_type = RTA_DST;
	rta->rta_len = RTA_LENGTH (addrlen);
	memcpy (RTA_DATA (rta), addr, addrlen);
	req.nh.nlmsg_len =
		NLMSG_ALIGN (req.nh.nlmsg_len) + rta->rta_len;

	/* set tun if index */
	rta = (struct rtattr *) 
		((char *)&req + NLMSG_ALIGN (req.nh.nlmsg_len));
	rta->rta_type = RTA_OIF;
	rta->rta_len = RTA_LENGTH (sizeof (tunifindex));
	memcpy (RTA_DATA (rta), &tunifindex, sizeof (tunifindex));
	req.nh.nlmsg_len =
		NLMSG_ALIGN (req.nh.nlmsg_len) + rta->rta_len;
	
	/* create netlink socket and send it */
	if ((fd = socket (AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) 
		return -1;

	memset (&sa_nl, 0, sizeof (sa_nl));
	sa_nl.nl_family = AF_NETLINK;

	if (sendto (fd, &req, req.nh.nlmsg_len, 0,
		    (struct sockaddr *)&sa_nl, sizeof (sa_nl)) < 0)
		return -1;

	close (fd);

	return 0;
}

int
add_route_to_tun (int af, void * addr, int prefixlen)
{
	return op_route_to_dev (RTM_NEWROUTE, af, addr, 
				prefixlen, LISP_TUNNAME);
}

int
del_route_to_tun (int af, void * addr, int prefixlen)
{
	return op_route_to_dev (RTM_DELROUTE, af, addr, 
				prefixlen, LISP_TUNNAME);
}

