/**
 * @file
 * esix ipv6 stack main file.
 *
 * @section LICENSE
 * Copyright (c) 2009, Floris Chabert, Simon Vetter. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AS IS'' AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO,PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR  
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS  SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ESIX_H
#define _ESIX_H
	#include <stddef.h>
	#define ESIX_BUFFER_SIZE 750 	//(750 * 4 = 3kB = 2 eth frames of 1500 bytes)
	#define ESIX_MAX_IPADDR	8 	//max number of IP addresses the node can have
	/**
	 * IPv6 address
	 */
	struct ip6_addr {
		u32_t	addr1;
		u32_t	addr2;
		u32_t	addr3;
		u32_t	addr4;
	};

	/**
	 * IP address table entry
	 */
	struct esix_ipaddr_table_row {
		struct 	ip6_addr addr;	//Actual ip address
		u8_t 	mask;		//netmask (in bits, counting the number of ones
					//starting from the left.
		u32_t	expiration_date;//date at which this entry expires.
					//0 : never expires (for now)
		//u32_t	preferred_exp_date;//date at which this address shouldn't be used if possible
		u8_t	scope;		//LINK_LOCAL_SCOPE or GLOBAL_SCOPE
	
	};


	/**
	 * IPv6 header 
	 */
	struct ip6_hdr {
		u32_t 	ver_tc_flowlabel; //version (4bits), trafic class (8 bits), flow label (20 bytes)
		u16_t 	payload_len;	//payload length (next headers + upper protocols)
		u8_t  	next_header;	//next header type
		u8_t  	hlimit; 	//hop limit
		u32_t	daddr1;		//first word of destination address
		u32_t	daddr2;		
		u32_t	daddr3;	
		u32_t	daddr4;
		u32_t	saddr1;		//first word of source address
		u32_t	saddr2;		
		u32_t	saddr3;	
		u32_t	saddr4;
		u32_t	data;
	};

	/**
	 * ICMPv6 header
	 */
	struct icmp6_hdr {
		u8_t	type;
		u8_t	code;
		u16_t	chksum;
		u32_t	data;
	};

	//list of IP protocol numbers (some are IPv6-specific)
	#define HOPOPT 	0x00
	#define ICMP	0x3A
	#define NONXT	0x3B
	#define TCP	0x06
	#define UDP	0x11
	#define FRAG	0x2C

	//list of ICMPv6 types
	#define DST_UNR	0x01	//Destination Unreachable
	#define TOO_BIG	0x02	//Packet Too Big
	#define TTL_EXP	0x03	//TTL Exceeded
	#define PARAM_P	0x04	//Parameter problem
	#define ECHO_RQ	0x80	//Echo request
	#define ECHO_RP	0x81	//Echo reply
	#define M_L_QRY	0x82	//Multicast listener query
	#define M_L_RPT	0x83	//Multicast listener report
	#define M_L_DNE	0x84	//Multicast listener done
	#define RTR_SOL 0x85	//Router sollicitation
	#define RTR_ADV 0x86	//Router advertisement
	#define NBR_SOL	0x87	//Neighbor sollicitation
	#define NBR_ADV	0x88	//Neighbor advertisement
	#define REDIR	0x89	//Redirect
	#define	MLDv2	0x90	//Multicast Listener Report (MLDv2) 

	#define LINK_LOCAL_SCOPE	0 
	#define GLOBAL_SCOPE	 	1

	
	
	void esix_received_frame(struct ip6_hdr *, int);
	void esix_received_icmp(struct icmp6_hdr *, int );
	int esix_add_to_active_addresses(struct esix_ipaddr_table_row *);
#endif
