#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
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
			memcpy (buf + pktlen, &(EXTRACT_INADDR(loc->loc_addr)),
				sizeof (struct in_addr));
			pktlen += sizeof (struct in_addr);
			break;

		case AF_INET6 :
			tmpafi = htons (LISP_AFI_IPV6);
			memcpy (ptr, &tmpafi, sizeof (tmpafi));
			pktlen += sizeof (tmpafi);
			memcpy (buf + pktlen, &(EXTRACT_IN6ADDR(loc->loc_addr)),
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
set_lisp_map_register (char * buf, int len, prefix_t * prefix, struct eid * eid)
{
	int pktlen = 0;
	char * ptr;
	listnode_t * li;
	struct locator * tmploc;
	struct lisp_record  * rec;
	struct lisp_locator * loc;
	struct lisp_map_register * reg;


	/* Set Register header */
	pktlen += sizeof (struct lisp_map_register);
	reg = (struct lisp_map_register *) buf;
	memset (reg, 0, sizeof (struct lisp_map_register));
	reg->type = 3;
	reg->record_count = 1;
	reg->P_flag = 1;
	reg->key_id = htons (1);
	reg->auth_data_len = htons (SHA_DIGEST_LENGTH);
	
	/* Set Record */
	pktlen += sizeof (struct lisp_record);
	rec = (struct lisp_record *) (buf + sizeof (struct lisp_map_register));
	memset (rec, 0, sizeof (struct lisp_record));
	rec->record_ttl = htonl (LISP_DEFAULT_RECORD_TTL);
	rec->locator_count = MYLIST_COUNT (lisp.loc_tuple);
	rec->eid_mask_len = prefix->bitlen;
	rec->eid_prefix_afi = htons (AF_TO_LISPAFI (prefix->family));
	
	ptr = (char *) (rec + 1);
	switch (prefix->family) {
	case AF_INET :
		pktlen += sizeof (struct in_addr);
		memcpy (ptr, &(prefix->add.sin), sizeof (struct in_addr));
		break;
	case AF_INET6 :
		pktlen += sizeof (struct in6_addr);
		memcpy (ptr, &(prefix->add.sin6), sizeof (struct in6_addr));
		break;
	}

	/* Set Locator */

	MYLIST_FOREACH (lisp.loc_tuple, li) {
		tmploc = (struct locator *) (li->data);

		pktlen += sizeof (struct lisp_locator);
		loc = (struct lisp_locator *) (buf + pktlen);
		memset (loc, 0, sizeof (struct lisp_locator));
		loc->priority = tmploc->priority;
		loc->weight = tmploc->weight;
		loc->m_priority = tmploc->m_priority;
		loc->m_weight = tmploc->m_weight;
		loc->R_flag = 1;
		loc->L_flag = 1;
		loc->afi = htons (EXTRACT_LISPAFI (tmploc->loc_addr));
	
		ptr = (char *) (loc + 1);
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
		}
	}
			  
	/* Calculate Auth Data */
	hmac (reg->auth_data, buf, pktlen, eid->authkey, strlen (eid->authkey));
	
	return pktlen;
}




int 
process_lisp_map_reply (struct lisp_map_reply * maprep)
{
	
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
	
	if (lisp.mapsrvaddr.ss_len == 0) {
		error_warn ("map server is node defined");
		return -1;
	}

	len = set_lisp_map_request (buf, sizeof (buf), prefix);
	if (len < 1) {
		return -1;
	}

	if (sendto (lisp.ctl_socket, buf, len, 0, 
		    (struct sockaddr *)&(lisp.mapsrvaddr), lisp.mapsrvaddr.ss_len) < 0)
		error_warn ("send map request failed");

	return 0;
}

int
send_map_reqgister (struct eid * eid)
{
	int len;
	char buf[LISP_MSG_BUF_LEN];
	listnode_t * li;
	prefix_t * prefix;

	MYLIST_FOREACH (eid->prefix_tuple, li) {
		prefix = (prefix_t *) (li->data);
		len = set_lisp_map_register (buf, sizeof (buf), prefix, eid);
		if (len < 0) {
			return -1;
		}
		if (sendto (lisp.ctl_socket, buf, len, 0,
			    (struct sockaddr *)&(lisp.mapsrvaddr),
			    lisp.mapsrvaddr.ss_len) < 0)
			error_warn ("send map register failed");
	}

	return 0;
}

