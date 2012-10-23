/*
 *	Common LISP parameters and structures
 *	based onf draft-ietf-lisp-23
 */

#ifndef _LISP_H_
#define _LISP_H_

#include <sys/types.h>
#include <openssl/sha.h>

#define PACKED __attribute__ ((__packed__))


/* Misc Parameters */
#define LISP_CONTROL_PORT	4342
#define LISP_CONTROL_CPORT	"4342"
#define LISP_DATA_PORT		4341
#define LISP_DATA_CPORT		"4341"

#define LISP_DEFAULT_RECORD_TTL	10	/* min */
#define LISP_MAX_KEYLEN		128



/* LISP Address Family Code */
#define LISP_AFI_IPV4		1
#define LISP_AFI_IPV6		2



/*
   LISP header

   +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   L   |N|L|E|V|I|flags|            Nonce/Map-Version                  |
   I   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   S   |                 Instance ID/Locator Status Bits               |
   P-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

struct lisp_hdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	flags:3;
	u_int8_t	I_flag:1;
	u_int8_t	V_flag:1;
	u_int8_t	E_flag:1;
	u_int8_t	L_flag:1;
	u_int8_t	N_flag:1;
#else 
	u_int8_t	N_flag:1;
	u_int8_t	L_flag:1;
	u_int8_t	E_flag:1;
	u_int8_t	V_flag:1;
	u_int8_t	I_flag:1;
	u_int8_t	flags:3;
#endif
	union {
		u_int8_t	lisp_hdr_un1_data8[3];
	} lisp_data_un1;
	union {
		u_int8_t	lisp_hdr_un2_data8[4];
		u_int32_t	lisp_hdr_un2_data32;
	} lisp_data_un2;
} PACKED;

/* if N bit is set to 1, 24 bit of 1st 32bit is "Nonce" */
#define lhdr_nonce		lisp_data_un1.lisp_hdr_un1_data8

/* if L bit is set to 1, 2nd 32bit is "Locator Status "*/
#define lhdr_loc_status	lisp_data_un2.lisp_hdr_un2_data32

/* if V bit is set to 1, 24bit of 1st 32bit is "Src/Dst Map version" */
//#define lhdr_src_map_version	lisp_data_un1._data12.lisp_hdr_un1_data12_1[0]
//#define lhdr_dst_map_version	lisp_data_un1._data12.lisp_hdr_un1_data12_2[1]

/* if I bit is set to 1, 2nd 32bit is "Instance ID and LSBs" */
#define lhdr_instance_id	lisp_data_un2.lisp_hdr_un2_data8
#define lhdr_lsb		lisp_data_un2.lisp_hdr_un2_data8[3]

 


/*	LISP Control Packet		*/


/*	LISP Control Packet Type	*/
#define LISP_MAP_RSVD		0
#define LISP_MAP_RQST		1
#define LISP_MAP_RPLY		2
#define LISP_MAP_RGST		3
#define LISP_MAP_NTFY		4
#define LISP_ECAP_CTL		8

/* LISP Encapsulated Control Packet */
struct lisp_control {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	rsv:3;
	u_int8_t	S_flag:1;
	u_int8_t	type:4;
#else
	u_int8_t	type:4;
	u_int8_t	S_flag:1;
	u_int8_t	rsv:3;
#endif
	u_int8_t	rsv2[3];
} PACKED;


/*
  LISP Map Request Meesage

 +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   |Type=1 |A|M|P|S|p|s|    Reserved     |   IRC   | Record Count  |
 H   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 D   |                         Nonce . . .                           |
 R   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   |                         . . . Nonce                           |
 +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |         Source-EID-AFI        |   Source EID Address  ...     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |         ITR-RLOC-AFI 1        |    ITR-RLOC Address 1  ...    |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                              ...                              |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |         ITR-RLOC-AFI n        |    ITR-RLOC Address n  ...    |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   / |   Reserved    | EID mask-len  |        EID-prefix-AFI         |
 Rec +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   \ |                       EID-prefix  ...                         |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                   Map-Reply Record  ...                       |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */


struct lisp_map_request {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	S_flag:1;
	u_int8_t	P_flag:1;
	u_int8_t	M_flag:1;
	u_int8_t	A_flag:1;
	u_int8_t	type:4;
#else
	u_int8_t	S_flag:1;
 	u_int8_t	P_flag:1;
	u_int8_t	M_flag:1;
	u_int8_t	A_flag:1;
	u_int8_t	type:4;
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	rsv:6;
	u_int8_t	s_flag:1;
	u_int8_t	p_flag:1;
#else
	u_int8_t	p_flag:1;
	u_int8_t	rsv:6;
	u_int8_t	s_flag:1;
#endif
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	irc:5;
	u_int8_t	rsv2:3;
#else
	u_int8_t	rsv2:3;
	u_int8_t	irc:5;
#endif
	u_int8_t	record_count;
	u_int32_t	nonce[2];
} PACKED;

struct lisp_map_request_record {
	u_int8_t	rsv;
	u_int8_t	eid_mask_len;
	u_int16_t	eid_prefix_afi;
} PACKED;

/*
  LISP Map Reply Message
  Reply is encapsulated by LISPCONTROL-IP-UDP

      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |Type=2 |P|E|S|          Reserved               | Record Count  |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                         Nonce . . .                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                         . . . Nonce                           |
  +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   |                          Record  TTL                          |
  |   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  R   | Locator Count | EID mask-len  | ACT |A|      Reserved         |
  e   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  c   | Rsvd  |  Map-Version Number   |       EID-prefix-AFI          |
  o   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  r   |                          EID-prefix                           |
  d   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  /|    Priority   |    Weight     |  M Priority   |   M Weight    |
  | L +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | o |        Unused Flags     |L|p|R|           Loc-AFI             |
  | c +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  \|                             Locator                           |
  +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

struct lisp_map_reply {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	rsv:1;
	u_int8_t	S_flag:1;
	u_int8_t	E_flag:1;
	u_int8_t	P_flag:1;
	u_int8_t	type:4;
#else
	u_int8_t	type:4;
	u_int8_t	P_flag:1;
	u_int8_t	E_flag:1;
	u_int8_t	S_flag:1;
	u_int8_t	rsv:1;
#endif
	u_int8_t	rsv2[2];
	u_int8_t	record_count;
	u_int32_t	nonce[2];
} PACKED;

/* ACT Field value in record (it is used only when loc count is 0)*/
#define LISP_MAPREPLY_ACT_NOACTION	0
#define LISP_MAPREPLY_ACT_NATIVE	1
#define LISP_MAPREPLY_ACT_SENDREQ	2
#define LISP_MAPREPLY_ACT_DROP		3


/*
  LISP Map Register Message

     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |Type=3 |P|            Reserved               |M| Record Count  |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                         Nonce . . .                           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                         . . . Nonce                           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |            Key ID             |  Authentication Data Length   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     ~                     Authentication Data                       ~
 +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   |                          Record  TTL                          |
 |   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 R   | Locator Count | EID mask-len  | ACT |A|      Reserved         |
 e   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 c   | Rsvd  |  Map-Version Number   |        EID-prefix-AFI         |
 o   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 r   |                          EID-prefix                           |
 d   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  /|    Priority   |    Weight     |  M Priority   |   M Weight    |
 | L +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | o |        Unused Flags     |L|p|R|           Loc-AFI             |
 | c +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  \|                             Locator                           |
 +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

struct lisp_locator {
	u_int8_t	priority;
	u_int8_t 	weight;
	u_int8_t 	m_priority;
	u_int8_t 	m_weight;
	u_int8_t 	rsv;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	R_flag:1;
	u_int8_t	p_flag:1;
	u_int8_t	L_flag:1;
	u_int8_t	rsv2:5;
#else
	u_int8_t	rsv2:5;
	u_int8_t	L_flag:1;
	u_int8_t	p_flag:1;
	u_int8_t	R_flag:1;
#endif
	u_int16_t 	afi;
} PACKED;

struct lisp_record {
	u_int32_t	record_ttl;
	u_int8_t	locator_count;
	u_int8_t	eid_mask_len;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	rsv:4;
	u_int8_t	A_flag:1;
	u_int8_t	act:3;
#else
	u_int8_t	act:3;
	u_int8_t	A_flag:1;
	u_int8_t	rsv:4;
#endif

	u_int8_t	rsv2;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	map_version_hi: 4;
	u_int8_t	rsv3:4;
#else
	u_int8_t	rsv3:4;
	u_int8_t	map_version_hi: 4;
#endif
	u_int8_t	map_version_low;
	u_int16_t	eid_prefix_afi;
} PACKED;


struct lisp_map_register {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	rsv:3;
	u_int8_t	P_flag:1;
	u_int8_t	type:4;
#else
	u_int8_t	type:4;
	u_int8_t	P_flag:1;
	u_int8_t	rsv:3;
#endif
	u_int8_t 	rsv2;
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	M_flag:1;
	u_int8_t	rsv3:7;
#else
	u_int8_t	rsv3:7;
 	u_int8_t	M_flag:1;
#endif
	u_int8_t	record_count;

	u_int32_t	nonce[2];
	u_int16_t	key_id;
	u_int16_t	auth_data_len;
	char		auth_data[SHA_DIGEST_LENGTH];
} PACKED;



/*
  LISP Map Notify Message
  
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |Type=4 |              Reserved                 | Record Count  |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                         Nonce . . .                           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                         . . . Nonce                           |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |            Key ID             |  Authentication Data Length   |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     ~                     Authentication Data                       ~
 +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   |                          Record  TTL                          |
 |   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 R   | Locator Count | EID mask-len  | ACT |A|      Reserved         |
 e   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 c   | Rsvd  |  Map-Version Number   |         EID-prefix-AFI        |
 o   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 r   |                          EID-prefix                           |
 d   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  /|    Priority   |    Weight     |  M Priority   |   M Weight    |
 | L +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | o |        Unused Flags     |L|p|R|           Loc-AFI             |
 | c +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  \|                             Locator                           |
 +-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

struct lisp_map_notify {
#if __BYTE_ORDER == __LITTLE_ENDIAN
	u_int8_t	rsv:4;
	u_int8_t	type:4;
#else
	u_int8_t	type:4;
	u_int8_t	rsv:4;
#endif
	u_int8_t	rsv2[2];
	u_int8_t	record_count;
	u_int32_t	nonce[2];
	u_int16_t	key_id;
	u_int16_t	auth_data_len;
	u_int8_t	auth_data[SHA_DIGEST_LENGTH];
} PACKED;





#endif /* _LISP_H_ */
