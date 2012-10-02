#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>


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


int
set_lisp_map_request (char * buf, int len, prefix_t * prefix)
{
	int irc = -1, pktlen = 0;
	char * ptr;
	u_int16_t tmpafi;
	listnode_t * linode;
	struct locator * tmploc;
	struct lisp_map_request * req;
	struct lisp_map_request_record * reqrec;
	
	/* Create Header of Request Message */
	pktlen = sizeof (struct lisp_map_request);
	req = (struct lisp_map_request *) buf;
	memset (req, 0, sizeof (struct lisp_map_request));
	req->type = LISP_MAP_RQST;
	req->record_count = 1;
	
	/* set Source EID */
	ptr = buf + pktlen;
	tmpafi = 0;
	memcpy (ptr, &tmpafi, sizeof (tmpafi));
	pktlen += sizeof (tmpafi);
	
	/* set ITR LOC Info */
	if (lisp.loc_tuple == NULL)
		return -1;

	MYLIST_FOREACH (lisp.loc_tuple, linode) {
		tmploc = (struct locator *)(linode->data);
		ptr = buf + pktlen;
		switch (EXTRACT_FAMILY (tmploc->loc_addr)) {
		case AF_INET : 
			tmpafi = htons (LISP_AFI_IPV4);
			memcpy (ptr, &tmpafi, sizeof (tmpafi));
			pktlen += sizeof (tmpafi);
			memcpy (buf + pktlen, &(EXTRACT_INADDR(tmploc->loc_addr)),
				sizeof (struct in_addr));
			pktlen += sizeof (struct in_addr);
			break;

		case AF_INET6 :
			tmpafi = htons (LISP_AFI_IPV6);
			memcpy (ptr, &tmpafi, sizeof (tmpafi));
			pktlen += sizeof (tmpafi);
			memcpy (buf + pktlen, &(EXTRACT_IN6ADDR(tmploc->loc_addr)),
				sizeof (struct in6_addr));
			pktlen += sizeof (struct in6_addr);
			break;
		}
		irc++;
	}
	if (irc < 0) {
		error_warn ("%s : ITR Count is -1 (0)", __func__);
		return -1;
	}
	req->irc = irc;
	
	/* Set Request Prefix */
	reqrec = (struct lisp_map_request_record *)(buf + pktlen);
	pktlen += sizeof (struct lisp_map_request_record);
	memset (reqrec, 0, sizeof (struct lisp_map_request_record));
	reqrec->eid_mask_len = prefix->bitlen;
	reqrec->eid_prefix_afi = htons (AF_TO_LISPAFI (prefix->family));

	/* EID AFI and Prefix*/
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

	return pktlen;
}




int 
set_lisp_map_register (char * buf, int len, struct eid * eid)
{
	int pktlen = 0;
	char * ptr;
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
	rec->locator_count = 1;
	rec->eid_mask_len = eid->prefix.bitlen;
	rec->eid_prefix_afi = htons (AF_TO_LISPAFI (eid->prefix.family));
	
	ptr = (char *) (rec + 1);
	switch (eid->prefix.family) {
	case AF_INET :
		pktlen += sizeof (struct in_addr);
		memcpy (ptr, &(eid->prefix.add.sin), sizeof (struct in_addr));
		break;
	case AF_INET6 :
		pktlen += sizeof (struct in6_addr);
		memcpy (ptr, &(eid->prefix.add.sin6), sizeof (struct in6_addr));
		break;
	}

	/* Set Locator */
	pktlen += sizeof (struct lisp_locator);
	loc = (struct lisp_locator *) (buf + sizeof (struct lisp_map_register) + 
				       sizeof (struct lisp_record) +
				       ((eid->prefix.family == AF_INET) ? 
					sizeof (struct in_addr) : 
					sizeof (struct in6_addr)));
	memset (loc, 0, sizeof (struct lisp_locator));
	loc->priority = eid->locator.priority;
	loc->weight = eid->locator.weight;
	loc->m_priority = eid->locator.m_priority;
	loc->m_weight = eid->locator.m_weight;
	loc->R_flag = 1;
	loc->L_flag = 1;
	loc->afi = htons (EXTRACT_LISPAFI (eid->locator.loc_addr));
	
	ptr = (char *) (loc + 1);
	switch (EXTRACT_FAMILY (eid->locator.loc_addr)) {
	case AF_INET :
		pktlen += sizeof (struct in_addr);
		memcpy (ptr, &(EXTRACT_INADDR (eid->locator.loc_addr)), 
			sizeof (struct in_addr));
		break;
	case AF_INET6 :
		pktlen += sizeof (struct in6_addr);
		memcpy (ptr, &(EXTRACT_IN6ADDR (eid->locator.loc_addr)), 
			sizeof (struct in6_addr));
		break;
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
hmac(char *md, void *buf, size_t size, char * key, int keylen)
{
	size_t reslen = keylen;

	unsigned char result[SHA_DIGEST_LENGTH];

	HMAC(EVP_sha1(), key, keylen, buf, size, result, (unsigned int *)&reslen);
	memcpy(md, result, SHA_DIGEST_LENGTH);
}

