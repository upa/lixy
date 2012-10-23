#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#define __FAVOR_BSD
#include <netinet/udp.h>

#include "lisp.h"
#include "map.h"
#include "error.h"
#include "common.h"
#include "sockaddrmacro.h"

#define EXTRACT_LISPAFI(sa) \
	((EXTRACT_FAMILY (sa) == AF_INET) ? LISP_AFI_IPV4 : LISP_AFI_IPV6)

#define AF_TO_LISPAFI(af) \
	((af == AF_INET) ? LISP_AFI_IPV4 : LISP_AFI_IPV6)

#define LISPAFI_TO_AF(af)				\
	(((af) == LISP_AFI_IPV4) ? AF_INET :		\
	 (((af) == LISP_AFI_IPV6) ? AF_INET6 : -1))

#define LISP_LOCATOR_PKT_LEN(LOCPKT)				\
	(sizeof (struct lisp_locator) +				\
	 (ntohs ((LOCPKT)->afi) == LISP_AFI_IPV4) ?		\
	 sizeof (struct in_addr) : sizeof (struct in6_addr))	\

#define LISP_RECORD_PKT_LEN(RECPKT)				\
	(sizeof (struct lisp_record) +				\
	(ntohs ((RECPKT)->eid_prefix_afi) == LISP_AFI_IPV4) ?	\
	 sizeof (struct in_addr) : sizeof (struct in6_addr))	\

void hmac(char *md, void *buf, size_t size, char * key, int keylen);

struct locator *
get_locator (int af) 
{
	listnode_t * ln;
	struct locator * loc;

	if (MYLIST_COUNT (lisp.loc_tuple) == 0) {
		return NULL;
	}

	MYLIST_FOREACH (lisp.loc_tuple, ln) {
		loc = (struct locator *) (ln->data);

		if (af == EXTRACT_FAMILY (loc->loc_addr) || af == 0)
			return loc;
	}

	return NULL;
}

void
set_ip_hdr (struct ip * ip, int data_len, struct in_addr src, struct in_addr dst)
{
	memset (ip, 0, sizeof (struct ip));

	ip->ip_hl	= 5;
	ip->ip_v	= 4;
	ip->ip_len	= htons (sizeof (struct ip) + 
				 sizeof (struct udphdr) + data_len);
	ip->ip_ttl	= 255;
	ip->ip_p	= IPPROTO_UDP;
	ip->ip_src	= src;
	ip->ip_dst	= dst;

	return;
}

void
set_ip6_hdr (struct ip6_hdr * ip6, int data_len, struct in6_addr src, struct in6_addr dst)
{
	memset (ip6, 0, sizeof (struct ip6_hdr));
	
	ip6->ip6_vfc	= 0x6E;
	ip6->ip6_plen	= htons (sizeof (struct udphdr) + data_len);
	ip6->ip6_nxt 	= IPPROTO_UDP;
	ip6->ip6_hlim	= 64;
	ip6->ip6_src	= src;
	ip6->ip6_dst	= dst;

	return;
}

void
set_udp_hdr (struct udphdr * udp, int u_len, int srcport, int dstport)
{
	memset (udp, 0, sizeof (struct udphdr));
	
	udp->uh_sport	= htons (srcport);
	udp->uh_dport	= htons (dstport);
	udp->uh_ulen	= htons (u_len);

	return;
}

u_int16_t
chksum (u_int16_t * buf, int len)
{
	int n = len / 2;
	u_int32_t sum;
	u_int16_t * p = buf;

	for (sum = 0; n > 0; n--)
		sum += *p++;

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ~sum;
}


int
set_lisp_map_request (char * buf, int len, prefix_t * prefix)
{
	/* 
	 * Map request message is always encapsulated 
	 * according to LISP encapsulated message format 
	 */

	int pktlen = 0;
	char * ptr;
	u_int16_t tmpafi;
	listnode_t * li;
	struct locator * loc;
	struct ip * ip;
	struct ip6_hdr * ip6;
	struct udphdr * udp;
	struct lisp_control * ctl;
	struct lisp_map_request * req;
	struct lisp_map_request_record * reqrec;
	
	/* set LISP Encaped control message header */
	ctl = (struct lisp_control *) (buf + pktlen);
	memset (ctl, 0, sizeof (struct lisp_control));
	ctl->type = LISP_ECAP_CTL;
	pktlen += sizeof (struct lisp_control);


	/* set IP header */
	loc = get_locator (prefix->family);
	if (loc == NULL) {
		error_warn ("locator is not set");
		return -1;
	}
	switch (prefix->family) {
	case AF_INET :
		ip = (struct ip *) (buf + pktlen);
		set_ip_hdr (ip, 0, 
			    EXTRACT_INADDR (*loc), 
			    prefix->add.sin);
		/* ip len is set later */
		pktlen += sizeof (struct ip);
		break;
		
	case AF_INET6 :
		ip6 = (struct ip6_hdr *) (buf + pktlen);
		set_ip6_hdr (ip6, 0, 
			     EXTRACT_IN6ADDR (*loc), 
			     prefix->add.sin6);
		/* ip6 len is set later */
		pktlen += sizeof (struct ip6_hdr);
		break;
	default :
		error_warn ("Invalid AI Family in destination eid prefix");
		return -1;
	}
	
	/* set udp header */
	udp = (struct udphdr *) (buf + pktlen);
	set_udp_hdr (udp, 0, LISP_CONTROL_PORT, LISP_CONTROL_PORT);
	/* udp length is set later */
	pktlen += sizeof (struct udphdr);
	
	/* set LISP Header of Request Message */
	req = (struct lisp_map_request *) (buf + pktlen);
	memset (req, 0, sizeof (struct lisp_map_request));
	req->type = LISP_MAP_RQST;
	req->record_count = 1;
	req->irc = MYLIST_COUNT (lisp.loc_tuple) - 1;	/* 0 means 1 ITR */
	set_nonce (req->nonce, sizeof (req->nonce));
	pktlen += sizeof (struct lisp_map_request);
	
	/* set Source EID */
	ptr = buf + pktlen;
	tmpafi = 0;
	memcpy (ptr, &tmpafi, sizeof (tmpafi));
	pktlen += sizeof (tmpafi);
	
	/* set ITR LOC Info */
	ptr = buf + pktlen;

	MYLIST_FOREACH (lisp.loc_tuple, li) {
		loc = (struct locator *) li->data;
		switch (EXTRACT_FAMILY (loc->loc_addr)) {
		case AF_INET : 
			tmpafi = htons (LISP_AFI_IPV4);
			memcpy (ptr, &tmpafi, sizeof (tmpafi));
			pktlen += sizeof (tmpafi);
			memcpy (buf + pktlen, 
				&(EXTRACT_INADDR(loc->loc_addr)),
				sizeof (struct in_addr));
			pktlen += sizeof (struct in_addr);
			break;

		case AF_INET6 :
			tmpafi = htons (LISP_AFI_IPV6);
			memcpy (ptr, &tmpafi, sizeof (tmpafi));
			pktlen += sizeof (tmpafi);
			memcpy (buf + pktlen,  
				&(EXTRACT_IN6ADDR(loc->loc_addr)),
				sizeof (struct in6_addr));
			pktlen += sizeof (struct in6_addr);
			break;
		default :
			error_warn ("Locator is not set");
			return -1;
		}
	}

	/* set Request Prefix Record */
	reqrec = (struct lisp_map_request_record *)(buf + pktlen);
	memset (reqrec, 0, sizeof (struct lisp_map_request_record));
	reqrec->eid_mask_len = prefix->bitlen;
	reqrec->eid_prefix_afi = htons (AF_TO_LISPAFI (prefix->family));
	pktlen += sizeof (struct lisp_map_request_record);

	/* set Request Prefix*/
	ptr = buf + pktlen;
	switch (prefix->family) {
	case AF_INET :
		memcpy (ptr, &(prefix->add.sin), sizeof (struct in_addr));
		pktlen += sizeof (struct in_addr);
		break;
	case AF_INET6 :
		memcpy (ptr, &(prefix->add.sin6), sizeof (struct in6_addr));
		pktlen += sizeof (struct in6_addr);
		break;
	}

	/* set ip len, udp len, and check sum */
	switch (EXTRACT_FAMILY (prefix->family)) {
	case AF_INET :
		ip->ip_len = htons (pktlen - sizeof (struct lisp_control));
		udp->uh_ulen = htons (pktlen 
				      - sizeof (struct lisp_control) 
				      - sizeof (struct ip));
		ip->ip_sum = chksum ((u_int16_t *)ip, sizeof (struct ip));
		break;
	case AF_INET6 :
		ip6->ip6_plen = htons (pktlen 
				       - sizeof (struct lisp_control)
				       - sizeof (struct ip6_hdr));
		udp->uh_ulen = htons (pktlen 
				      - sizeof (struct lisp_control)
				      - sizeof (struct ip6_hdr));
		break;
	}

	return pktlen;
}




int 
set_lisp_map_register (char * buf, int len, prefix_t * prefix, char * key)

{
	int pktlen = 0;
	char * ptr;
	listnode_t * li;
	struct locator * tmploc;
//	struct ip * ip;
//	struct ip6_hdr * ip6;
//	struct udphdr * udp;
//	struct lisp_control * ctl;
	struct lisp_record  * rec;
	struct lisp_locator * loc;
	struct lisp_map_register * reg;

#if 0
	/* set LISP Encaped control message header */
	ctl = (struct lisp_control *)(buf + pktlen);
	memset (ctl, 0, sizeof (struct lisp_control));
	ctl->type = LISP_ECAP_CTL;
	pktlen += sizeof (struct lisp_control);

	/* set IP header */
	tmploc = get_locator (prefix->family);
	if (tmploc == NULL) {
		error_warn ("%s : locator is not set", __func__);
		return -1;
	}
	switch (prefix->family) {
	case AF_INET : 
		ip = (struct ip *) (buf + pktlen);
		set_ip_hdr (ip, 0, EXTRACT_INADDR (*tmploc),
			    prefix->add.sin);
		/* ip len is set later */
		pktlen += sizeof (struct ip);
		break;

	case AF_INET6 :
		ip6 = (struct ip6_hdr *)(buf + pktlen);
		set_ip6_hdr (ip6, 0, EXTRACT_IN6ADDR (*tmploc), 
			     prefix->add.sin6);
		/* ip6 len is set later */
		pktlen += sizeof (struct ip6_hdr);
		break;
	default :
		error_warn ("%s: Invalid AI Family in destination eid prefix",
			    __func__);
		return -1;
	}

	/* set UDP header */
	udp = (struct udphdr *)(buf + pktlen);
	set_udp_hdr (udp, 0, LISP_CONTROL_PORT, LISP_CONTROL_PORT);
	/* udp length is set later */
	pktlen += sizeof (struct udphdr);
#endif

	/* Set Register header */
	reg = (struct lisp_map_register *) (buf + pktlen);
	memset (reg, 0, sizeof (struct lisp_map_register));
	reg->type = LISP_MAP_RGST;
	reg->record_count = 1;
	reg->P_flag = 1;
	reg->key_id = htons (1);
	reg->auth_data_len = htons (SHA_DIGEST_LENGTH);
	pktlen += sizeof (struct lisp_map_register);	

	/* Set Record */
	rec = (struct lisp_record *) (buf + pktlen);
	memset (rec, 0, sizeof (struct lisp_record));
	rec->record_ttl = htonl (LISP_DEFAULT_RECORD_TTL);
	rec->locator_count = MYLIST_COUNT (lisp.loc_tuple);
	rec->eid_mask_len = prefix->bitlen;
	rec->eid_prefix_afi = htons (AF_TO_LISPAFI (prefix->family));
	set_nonce (reg->nonce, sizeof (reg->nonce));
	pktlen += sizeof (struct lisp_record);
	
	ptr = buf + pktlen;
	switch (prefix->family) {
	case AF_INET :
		memcpy (ptr, &(prefix->add.sin), sizeof (struct in_addr));
		pktlen += sizeof (struct in_addr);
		break;
	case AF_INET6 :
		memcpy (ptr, &(prefix->add.sin6), sizeof (struct in6_addr));
		pktlen += sizeof (struct in6_addr);
		break;
	}


	/* Set Locator */

	MYLIST_FOREACH (lisp.loc_tuple, li) {
		tmploc = (struct locator *) (li->data);

		loc = (struct lisp_locator *) (buf + pktlen);
		memset (loc, 0, sizeof (struct lisp_locator));
		loc->priority = tmploc->priority;
		loc->weight = tmploc->weight;
		loc->m_priority = tmploc->m_priority;
		loc->m_weight = tmploc->m_weight;
		loc->R_flag = 1;
		loc->L_flag = 1;
		loc->afi = htons (AF_TO_LISPAFI 
				  (EXTRACT_FAMILY (tmploc->loc_addr)));
		pktlen += sizeof (struct lisp_locator);

		ptr = buf + pktlen;
		switch (EXTRACT_FAMILY (tmploc->loc_addr)) {
		case AF_INET :
			pktlen += sizeof (struct in_addr);
			memcpy (ptr, &(EXTRACT_INADDR (tmploc->loc_addr)), 
				sizeof (struct in_addr));
			break;
		case AF_INET6 :
			pktlen += sizeof (struct in6_addr);
			memcpy (ptr, &(EXTRACT_IN6ADDR (tmploc->loc_addr)), 
				sizeof (struct in6_addr));
			break;
		default :
			error_warn ("%s: unknown family locator %d", 
				    EXTRACT_FAMILY (tmploc->loc_addr));
			return -1;
		}
	}
			  
#if 0
	/* set ip len, udp len, and checksum */
	switch (EXTRACT_FAMILY (prefix->family)) {
	case AF_INET :
		ip->ip_len = htons (pktlen - sizeof (struct lisp_control));
		udp->uh_ulen = htons (pktlen 
				      - sizeof (struct lisp_control)
				      - sizeof (struct ip));
		break;
	case AF_INET6 :
		ip6->ip6_plen = htons (pktlen
				       - sizeof (struct lisp_control)
				       - sizeof (struct ip6_hdr));
		udp->uh_ulen = htons (pktlen
				      - sizeof (struct lisp_control)
				      - sizeof (struct ip6_hdr));
		break;
	}
#endif

	/* Calculate Auth Data */
	hmac (reg->auth_data, buf, pktlen, key, strlen (key));
	
	return pktlen;
}



int
set_lisp_map_reply (char * buf, int len, prefix_t * prefix, u_int32_t * nonce)
{
	int pktlen = 0;
	char * ptr;
	listnode_t * li;
	struct locator * tmploc;
	struct lisp_map_reply * rep;
	struct lisp_record * rec;
	struct lisp_locator * loc;

        /* 
	 * map reply is used for RLOC Probing.
	 * so, Probe bit is always set 1, and nonce is copied from
	 * probing map request.
	 */

	/* Set Map Replay Header */
	rep = (struct lisp_map_reply *)(buf + pktlen);
	memset (rep, 0, sizeof (struct lisp_map_reply));
	rep->type = LISP_MAP_RPLY;
	rep->P_flag = 1;	
	rep->record_count = 1;
	memcpy (rep->nonce, nonce, sizeof (rep->nonce));
	pktlen += sizeof (struct lisp_map_reply);

	/* Set Record */
	rec = (struct lisp_record *)(buf + pktlen);
	memset (rec, 0, sizeof (struct lisp_record));
	rec->record_ttl = htonl (LISP_DEFAULT_RECORD_TTL);
	rec->locator_count = MYLIST_COUNT (lisp.loc_tuple);
	rec->eid_mask_len = prefix->bitlen;
	rec->eid_prefix_afi = htons (AF_TO_LISPAFI (prefix->family));
	pktlen += sizeof (struct lisp_record);

	ptr = buf + pktlen;
	switch (prefix->family) {
	case AF_INET :
		memcpy (ptr, &(prefix->add.sin), sizeof (struct in_addr));
		pktlen += sizeof (struct in_addr);
		break;
	case AF_INET6 :
		memcpy (ptr, &(prefix->add.sin6), sizeof (struct in6_addr));
		pktlen += sizeof (struct in6_addr);
		break;
	}

	/* Set Locator */
        MYLIST_FOREACH (lisp.loc_tuple, li) {
                tmploc = (struct locator *) (li->data);

                loc = (struct lisp_locator *) (buf + pktlen);
                memset (loc, 0, sizeof (struct lisp_locator));
                loc->priority = tmploc->priority;
                loc->weight = tmploc->weight;
                loc->m_priority = tmploc->m_priority;
                loc->m_weight = tmploc->m_weight;
                loc->R_flag = 1;
                loc->L_flag = 1;
                loc->afi = htons (AF_TO_LISPAFI
                                  (EXTRACT_FAMILY (tmploc->loc_addr)));
                pktlen += sizeof (struct lisp_locator);

                ptr = buf + pktlen;
                switch (EXTRACT_FAMILY (tmploc->loc_addr)) {
                case AF_INET :
                        pktlen += sizeof (struct in_addr);
                        memcpy (ptr, &(EXTRACT_INADDR (tmploc->loc_addr)),
                                sizeof (struct in_addr));
                        break;
                case AF_INET6 :
                        pktlen += sizeof (struct in6_addr);
                        memcpy (ptr, &(EXTRACT_IN6ADDR (tmploc->loc_addr)),
                                sizeof (struct in6_addr));
                        break;
                default :
                        error_warn ("%s: unknown family locator %d",
                                    EXTRACT_FAMILY (tmploc->loc_addr));
                        return -1;
                }
        }

	return pktlen;
}


void
process_lisp_map_reply_record_loc_is_zero (struct lisp_record * rec)
{
	/* Process Negative Cache */

	char addrbuf[ADDRBUFLEN];
	prefix_t * prefix;
	struct mapnode * mn;

	prefix = (prefix_t *) malloc (sizeof (prefix_t));
	memset (prefix, 0, sizeof (prefix_t));

	mn = (struct mapnode *) malloc (sizeof (struct mapnode));
	memset (mn, 0, sizeof (struct mapnode));
	mn->ttl = MAPTTL_DEFAULT;

	ADDRTOPREFIX (LISPAFI_TO_AF (htons (rec->eid_prefix_afi)),
		      *(rec + 1), rec->eid_mask_len, prefix);

	switch (rec->act) {
	case LISP_MAPREPLY_ACT_NOACTION :
		break;

	case LISP_MAPREPLY_ACT_NATIVE :
		mn->state = MAPSTATE_NEGATIVE;
		if (update_mapnode (lisp.rib, prefix, mn) == NULL) {
			error_warn ("%s: can not update maptalbe", __func__);
			free (prefix);
			free (mn);
		}
		break;

	case LISP_MAPREPLY_ACT_SENDREQ :
		send_map_request (prefix);
		free (prefix);
		break;

	case LISP_MAPREPLY_ACT_DROP :
		mn->state = MAPSTATE_DROP;
		if (update_mapnode (lisp.rib, prefix, mn) == NULL) {
			error_warn ("%s: can not update maptalbe", __func__);
			free (prefix);
			free (mn);
		}
		break;

	default :
		error_warn ("unknown MAP Reply Action %d for %s", rec->act, 
			    inet_pton (LISPAFI_TO_AF (
					       ntohs (rec->eid_prefix_afi)),
				       (char *)(rec + 1), addrbuf));
	}

	return;
}

struct locator 
lisp_locator_pkt_to_locator (struct lisp_locator * lisploc)
{
	struct locator loc;

	memset (&loc, 0, sizeof (loc));
	FILL_SOCKADDR (LISPAFI_TO_AF (ntohs (lisploc->afi)), 
		       &(loc.loc_addr), *(lisploc + 1));
	EXTRACT_PORT (loc.loc_addr) = htons (LISP_DATA_PORT);
	loc.priority = lisploc->priority;
	loc.weight = lisploc->weight;
	loc.m_priority = lisploc->m_priority;
	loc.m_weight = lisploc->m_weight;

	return loc;
}


void
process_lisp_map_message (char * pkt)
{
	struct ip * ip;
        struct lisp_control * ctl;

        ctl = (struct lisp_control *) pkt;

	if (ctl->type == LISP_ECAP_CTL) {
		ip = (struct ip *)(pkt + sizeof (struct lisp_control));
		switch (ip->ip_v) {
		case 4 :
			ctl = (struct lisp_control *)
				(pkt
				 + sizeof (struct lisp_control)
				 + sizeof (struct ip)
				 + sizeof (struct udphdr));
			break;

		case 6 :
			ctl = (struct lisp_control *)
				(pkt
				 + sizeof (struct lisp_control)
				 + sizeof (struct ip6_hdr)
				 + sizeof (struct udphdr));
			break;
		default :
			error_warn ("%s: Invalid inner IP version in"
				    "Encapsulated Control Message %d",
				    ip->ip_v);
			return;
			break;
		}
	}

        switch (ctl->type) {
        case LISP_MAP_RPLY :
                process_lisp_map_reply (pkt);
                break;

        case LISP_MAP_RQST :
		error_warn ("%s: Map request is comm", __func__);
                process_lisp_map_request (pkt);
                break;

        default :
                error_warn ("%s: invalid lisp message type %d",
                            __func__, ctl->type);
        }

        return;
}

int
process_lisp_map_request (char * pkt)
{
	int n, pktlen = 0;
	char * eid_addr;
	prefix_t prefix;
	struct lisp_map_request * req;
	struct lisp_map_request_record * qrec;

	memset (&prefix, 0, sizeof (prefix));

	/* 
	 * in this implementation,
	 * map request is always recieved for RLOC Probing,
	 */

	req = (struct lisp_map_request *) pkt;
	if (req->P_flag == 0) {
		error_warn ("%s: received map request message "
			    "but Probe flag is 0", __func__);
		return 0;
	}
	pktlen += sizeof (struct lisp_map_request);
	printf ("pktlen is %d\n", pktlen);

#define REQUEST_LOC_LEN(locafi) \
	((ntohs (*((u_int16_t *)(locafi))) == LISP_AFI_IPV4) ?		\
	 2 + sizeof (struct in_addr) :					\
	 (ntohs (*((u_int16_t *)(locafi))) == LISP_AFI_IPV6) ?		\
	 2 + sizeof (struct in6_addr) : 2)				\
	
        /* Source EID AFI & address */
	pktlen += REQUEST_LOC_LEN (pkt + pktlen);
	printf ("pktlen is %d\n", pktlen);
	
        /* ITR-RLOC-AFI and Address */
	for (n = 0; n <= req->irc; n++)  {
		pktlen += REQUEST_LOC_LEN (pkt + pktlen);
		printf ("pktlen is %d\n", pktlen);
	}

	qrec = (struct lisp_map_request_record *)(pkt + pktlen);
	pktlen += sizeof (struct lisp_map_request_record);
	eid_addr = pkt + pktlen;
	
	ADDRTOPREFIX (LISPAFI_TO_AF(ntohs (qrec->eid_prefix_afi)), 
		      *eid_addr, qrec->eid_mask_len, &prefix);

	char addrbuf[ADDRBUFLEN];
	inet_ntop (prefix.family, &(prefix.add), 
		   addrbuf, sizeof (addrbuf));
	printf ("%s: prefix is %s/%d, family is %d\n", 
		__func__, addrbuf, prefix.bitlen, prefix.family);


	int len;
	if ((len = send_map_reply (&prefix, req->nonce)) > 0)
		error_warn ("%s: send map reply! %d", __func__, len);
	else 
		error_warn ("%s: send map reply failed! %d", __func__, len);

	return 0;
}

int 
process_lisp_map_reply (char * pkt)
{
	/* Probe, Echo-nonce, Security bit is not supported yet */
	
	int n, i, pktlen;
	prefix_t * prefix;
	struct mapnode * mn;
	struct lisp_map_reply * rep;
	struct lisp_record * rec;
	struct lisp_locator * lisploc, * tmplisploc;

	/* Process Records */
	rep = (struct lisp_map_reply *) pkt;
	pktlen = sizeof (struct lisp_map_reply);
	for (n = 0; n < rep->record_count; n++) {
		rec = (struct lisp_record *) (pkt + pktlen);
		pktlen += LISP_RECORD_PKT_LEN (rec);
		if (rec->locator_count == 0) {
			/* Negative Cache */ 
			process_lisp_map_reply_record_loc_is_zero (rec);
			continue;
		}
		
		/* Select Locator and */
		lisploc = tmplisploc = (struct lisp_locator *)(pkt + pktlen);
		for (i = 0; i < rec->locator_count; i++) {
			if (lisploc->priority < tmplisploc->priority)
				lisploc = tmplisploc;
			pktlen += LISP_LOCATOR_PKT_LEN (tmplisploc);
			tmplisploc = (struct lisp_locator *)(pkt + pktlen);
		}

		/* register lcoator to maptable */
		prefix = (prefix_t *) malloc (sizeof (prefix_t));
		memset (prefix, 0, sizeof (prefix_t));
		ADDRTOPREFIX (LISPAFI_TO_AF (ntohs (rec->eid_prefix_afi)),
			      *(rec + 1), rec->eid_mask_len, prefix);
		mn = (struct mapnode *) malloc (sizeof (struct mapnode));
		memset (mn, 0, sizeof (mn));
		mn->state = MAPSTATE_ACTIVE;
		mn->ttl = ntohl (rec->record_ttl) * 60;
		mn->locator = lisp_locator_pkt_to_locator (lisploc);
		mn->addr = mn->locator.loc_addr;
		if (update_mapnode (lisp.rib, prefix, mn) == NULL) {
			error_warn ("%s: can not update maptable", __func__);
			free (prefix);
			free (mn);
		}
	}
	
	return 0;
}


void
hmac (char *md, void *buf, size_t size, char * key, int keylen)
{
	size_t reslen = keylen;

	unsigned char result[SHA_DIGEST_LENGTH];

	HMAC(EVP_sha1(), key, keylen, buf, size, result, (unsigned int *)&reslen);
	memcpy(md, result, SHA_DIGEST_LENGTH);
}


int
send_map_request (prefix_t * prefix)
{
	int len;
	char buf[LISP_MSG_BUF_LEN];
	
	if (EXTRACT_FAMILY (lisp.mapsrvaddr) == 0) {
		error_warn ("%s: map server is not defined", __func__);
		return -1;
	}

	len = set_lisp_map_request (buf, sizeof (buf), prefix);
	if (len < 1) {
		return -1;
	}

	if (sendto (lisp.ctl_socket, buf, len, 0, 
		    (struct sockaddr *)&(lisp.mapsrvaddr), 
		    EXTRACT_SALEN (lisp.mapsrvaddr)) < 0)
		error_warn ("send map request failed");

	return 0;
}

int
send_map_register (struct eid * eid)
{
	int len;
	char buf[LISP_MSG_BUF_LEN];
	listnode_t * li;
	prefix_t * prefix;

	MYLIST_FOREACH (eid->prefix_tuple, li) {
		prefix = (prefix_t *) (li->data);
		len = set_lisp_map_register (buf, sizeof (buf), 
					     prefix, eid->authkey);
		if (len < 0) {
			return -1;
		}
		if (sendto (lisp.ctl_socket, buf, len, 0,
			    (struct sockaddr *)&(lisp.mapsrvaddr),
			    EXTRACT_SALEN (lisp.mapsrvaddr)) < 0) {
			error_warn ("%s: send map register failed", __func__);
			return -1;
		}
	}

	return 0;
}


int
send_map_reply (prefix_t * prefix, u_int32_t * nonce)
{
	int len;
	char buf[LISP_MSG_BUF_LEN];
	
	len = set_lisp_map_reply (buf, sizeof (buf), prefix, nonce);
	if (len < 0)
		return -1;

	if (sendto (lisp.ctl_socket, buf, len, 0,
		    (struct sockaddr *)&(lisp.mapsrvaddr),
		    EXTRACT_SALEN (lisp.mapsrvaddr)) < 0) {
		error_warn ("%s: send map reply failed", __func__);
		return -1;
	}

	return len;
}

void
set_nonce (void * ptr, int size)
{
	int n;
	char * p = (char *) ptr;

	srand (time (NULL));
	
	for (n = 0; n < size; n++) 
		* (p + n) = rand () % 0xFF;

	return;
}
