#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <openssh/sha.h>
#include <openssl/hmac.h>

#include "lisp.h"
#include "map.h"
#include "error.h"
#include "sockaddrmacro.h"

#define EXTRACT_LISPAFI(sa) \
	((EXTRACT_FAMILY (sa) == AF_INET) ? LISP_AFI_IPV4	\
	 : LISP_AFI_IPV6)

void hmac(char *md, void *buf, size_t size, char * key, int keylen);


int
set_lisp_map_request (char * buf, int len, struct prefix * eid)
{
	int irc = -1, pktlen = 0;
	char * ptr;
	struct addr_tuple * tmploc;
	struct lisp_map_request * req;
	
	/* Create Header of Request Message */
	pktlen = sizeof (struct lisp_map_request) + 2;
	memset (req, 0, sizeof (struct lisp_map_request));
	req->type = LISP_MAP_RQST;
	
	/* set Source EID */
	ptr = buf + pktlen;
	* ((u_int8_t *) ptr) = 0;
	pktlen += 2;
	
	/* set ITR LOC Info */
	LL_FOREACH (lisp.loc_tuple, tmploc) {
		ptr = buf + pktlen;
		switch (EXTRACT_FAMILY (tmploc->addr)) {
		case AF_INET : 
			*((u_int16_t *) ptr) = htons (LISP_AFI_IPV4);
			ptr += 2;
			memcpy (ptr, &(EXTRACT_INADDR(tmploc->addr)),
				sizeof (struct in_addr));
			pktlen += 2 + sizeof (struct in_addr);
			break;

		case AF_INET6 :
			*((u_int16_t *) ptr) = htons (LISP_AFI_IPV6);
			ptr += 2;
			memcpy (ptr, &(EXTRACT_IN6ADDR(tmploc->addr)),
				sizeof (struct in6_addr));
			pktlen += sizeof (struct in6_addr);
			break;
		}
		* ((u_int8_t *) ptr ) = 0;
		irc++;
	}
	if (--irc < 0) {
		error_warn ("%s : ITR Count is -1 (0)", __func__);
		return -1;
	}
	req->irc = irc;
	
	/* Set EID */
	ptr = buf + (++pktlen);	       		/* reseved */

	*((u_int8_t *)ptr) = eid->mask_len;	/* EID Mask-len */
	ptr = buf + (++pktlen);	

	/* EID AFI and Prefix*/
	switch (EXTRACT_FAMILY (eid->addr)) {
	case AF_INET :
		*((u_int16_t *)ptr) = htons (LISP_AFI_IPV4);
		ptr = buf + (++pktlen);	
		memcpy (ptr, &(EXTRACT_INADDR (eid->addr)), sizeof (struct in_addr));
		pktlen += sizeof (struct in_addr);
		break;
	case AF_INET6 :
		*((u_int16_t *)ptr) = htons (LISP_AFI_IPV6);
		ptr = buf + (++pktlen);	
		memcpy (ptr, &(EXTRACT_IN6ADDR (eid->addr)), sizeof (struct in6_addr));
		pktlen += sizeof (struct in6_addr);
		break;
	}

	return pktlen;
}




int 
set_lisp_map_register (char * buf, int len, struct eid_tuple * eid)
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
	reg->type = LISP_MAP_RGST;
	reg->record_count = 1;
	reg->P_flag = 1;
	reg->key_id = htons (1);
	reg->auth_data_len = SHA_DIGEST_LENGTH;
	
	/* Set Record */
	pktlen += sizeof (struct lisp_record);
	rec = (struct lisp_record *) (buf + sizeof (struct lisp_map_register));
	memset (rec, 0, sizeof (struct lisp_record));
	rec->record_ttl = htonl (LISP_DEFAULT_RECORD_TTL);
	rec->locator_count = 1;
	rec->eid_mask_len = eid->eid_prefix.mask_len;
	rec->eid_prefix_afi = htons (EXTRACT_LISPAFI (eid->eid_prefix.addr));
	
	/* Set Locator */
	pktlen += sizeof (struct lisp_locator);
	loc = (struct lisp_locator *) (buf + sizeof (struct lisp_map_register) + 
				       EXTRACT_ADDRLEN (eid->eid_prefix.addr));
	memset (loc, 0, sizeof (struct lisp_record));
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

