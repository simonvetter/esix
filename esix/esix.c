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

#include "config.h"
#include "include/esix.h"
//general purpose buffer, mainly used to build packets/
//static int esix_buffer[ESIX_BUFFER_SIZE];

//this table contains every ip address assigned to the system.
static struct 	esix_ipaddr_table_row *addrs[ESIX_MAX_IPADDR];

//this table contains every routes assigned to the system
static struct 	esix_route_table_row *routes[ESIX_MAX_RT];

//stores our mac addr, for now...
static u16_t mac_addr[3];

/**
 * esix_init : sets up the esix stack.
 */
void esix_init(void)
{
	int i,j;
	for(i=0; i<ESIX_MAX_IPADDR; i++)
		addrs[i] = NULL;

	for(i=0; i<ESIX_MAX_RT; i++)
		routes[i] = NULL;


	//change here to get a dest mac address out of your ethernet
	//driver if needed. Implementing ether_get_mac_addr() in your driver
	//is the best way.
	//output must be in network order (big endian)

	ether_get_mac_addr(mac_addr);

	add_basic_addr_routes(mac_addr, INTERFACE, 1500);
	//esix_send_router_sollicitation(INTERFACE);
}

/**
 * esix_received_frame : processes incoming packets, does sanity checks,
 * then passes the payload to the corresponding upper layer.
 */
void esix_received_frame(struct ip6_hdr *hdr, int length)
{
	int i, pkt_for_us;
	//check if we have enough data to at least read the header
	//and if we actually have an IPv6 packet
	if((length < 40)  || 
		((hdr->ver_tc_flowlabel&hton32(0xf0000000)) !=  hton32(0x06 << 28)))
		return; 

	//now check if the ethernet frame is long enough to carry the entire ipv6 packet
	if(length < (ntoh16(hdr->payload_len) + 40))
		return;
	
	//check if the packet belongs to us
	pkt_for_us = 0;
	for(i=0; i<ESIX_MAX_IPADDR;i++)
	{
		//go through every entry of our address table and check word by word
		if( (addrs[i] != NULL) &&
			( hdr->daddr1 == (addrs[i]->addr.addr1) ) &&
			( hdr->daddr2 == (addrs[i]->addr.addr2) ) &&
			( hdr->daddr3 == (addrs[i]->addr.addr3) ) &&
			( hdr->daddr4 == (addrs[i]->addr.addr4) ))
		{
			pkt_for_us = 1;
			break;
		}

	}

	//drop the packet in case it doesn't
	if(pkt_for_us==0)
		return;

	//check the hop limit value (should be > 0)
	if(hdr->hlimit == 0)
	{
		esix_send_ttl_expired(hdr);
		return;
	}

	//determine what to do next
	switch(hdr->next_header)
	{
		case ICMP:
			esix_received_icmp((struct icmp6_hdr *) &hdr->data, 
				ntoh16(hdr->payload_len), hdr);
			break;

		//unknown (unimplemented) IP type
		default:
			return;
			break;
	}
}

/**
 * esix_received_icmp : handles icmp packets.
 */
void esix_received_icmp(struct icmp6_hdr *icmp_hdr, int length, struct ip6_hdr *ip_hdr )
{
	//check if we have enough bytes to read the ICMP header
	if(length < 4)
		return;

	//determine what to do next
	switch(icmp_hdr->type)
	{
		case NBR_SOL:
			break;
		case RTR_ADV:
			esix_parse_rtr_adv(
				(struct icmp6_rtr_adv *) &icmp_hdr->data, length - 4, ip_hdr);
			break;
		case ECHO_RQ: 
			break;
		default:	
			return;
	}
}


/**
 * esix_add_to_active_addresses : adds the given IP address to the table. Returns 1 on success.
 */
int esix_add_to_active_addresses(struct esix_ipaddr_table_row *row)
{
	int i=0;

	//look for an empty place where to store our entry.
	while(i<ESIX_MAX_IPADDR)
	{
		if(addrs[i] == NULL)
		{
			addrs[i] = row;
			return 1;
		}
		i++;
	}
	//sorry dude, table was full.
	return 0;
}

/**
 * esix_add_to_active_routes : adds the given route to the routing table. Returns 1 on success.
 */
int esix_add_to_active_routes(struct esix_route_table_row *row)
{
	int i=0;

	while(i<ESIX_MAX_RT)
	{
		if(routes[i]	== NULL)
		{
			routes[i] = row;
			return 1;
		}
		i++;
	}	
	//sorry dude, table was full.
	return 0;
}

/**
 * hton16 : converts host endianess to big endian (network order) 
 */
u16_t hton16(u16_t v)
{
	if(ENDIANESS)
		return v;

	u8_t * tmp	= (u8_t *) &v;
	return (tmp[0] << 8 | tmp[1]);
}

/**
 * hton32 : converts host endianess to big endian (network order) 
 */
u32_t hton32(u32_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 24) & 0xff000000) |
	       ((v << 8) & 0x00ff0000) |
	       ((v >> 8) & 0x0000ff00) |
	       ((v >> 24) & 0x000000ff);
}

/**
 * ntoh16 : converts network order to host endianess
 */
u16_t ntoh16(u16_t v)
{
	if(ENDIANESS)
		return v;

	//bug due to alignment weirdiness
	//u8_t * tmp	= (u8_t *) &v;
	//return (tmp[1] << 8 | tmp[0]);
	return ((v << 8) & 0xff00) | ((v >> 8) & 0x00ff);
}

/**
 * ntoh32 : converts network order to host endianess 
 */
u32_t ntoh32(u32_t v)
{
	if(ENDIANESS)
		return v;

	return ((v << 24) & 0xff000000) |
	       ((v << 8) & 0x00ff0000) |
	       ((v >> 8) & 0x0000ff00) |
	       ((v >> 24) & 0x000000ff);
}

/**
 * add_basic_addr_routes : adds a link local address/route based on the MAC address
 * and joins the all-nodes mcast group
 */
void add_basic_addr_routes(u16_t *mac_addr, int intf_index, int intf_mtu)
{
	//builds our link local and associated multicast addresses
	//from the MAC address given by the L2 layer.

	//unicast link local
	struct esix_ipaddr_table_row *ucast_ll = MALLOC (sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	ucast_ll->addr.addr1	= hton32(0xfe800000); // fe80/8 is the link local unicast prefix 
	ucast_ll->addr.addr2	= hton32(0x00000000);

	//last 64 bits
	u8_t	*tmp	= (u8_t*) &mac_addr[1] ; //split a half word in two bytes so we can use them
						//separately
	ucast_ll->addr.addr3	= hton32(
					mac_addr[0] << 16 | tmp[1] << 8 | 0xff);
	ucast_ll->addr.addr4	= hton32(0xfe << 24 | tmp[0] << 16 | mac_addr[2]);

	//this one never expires
	ucast_ll->expiration_date	= 0x0;
	ucast_ll->scope			= LINK_LOCAL_SCOPE;

	//multicast link local associated address (for neighbor discovery)
	struct esix_ipaddr_table_row *mcast_ll = MALLOC (sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	mcast_ll->addr.addr1	= hton32(0xff020000);
	mcast_ll->addr.addr2	= hton32(0x00000000);

	//last 64 bits
	mcast_ll->addr.addr3	= hton32(0x00000001);

	//TODO: endianess support & testing 
	//struct ether_addr with some unions in it avoiding endianess weirdiness?
	/*if(ENDIANESS)
		mcast_ll->addr.addr4	= hton32(0xff << 24 | tmp[1] << 16 | mac_addr[2]);
	else*/
	mcast_ll->addr.addr4	= hton32(0xff << 24 | tmp[0] << 16 | mac_addr[2]);

	//this one never expires
	mcast_ll->expiration_date	= 0x0;
	mcast_ll->scope			= MCAST_SCOPE;

	//multicast all-nodes (for router advertisements)
	
	struct esix_ipaddr_table_row *mcast_all = MALLOC (sizeof (struct esix_ipaddr_table_row)); 

	//first 64 bits
	mcast_all->addr.addr1	= hton32(0xff020000);
	mcast_all->addr.addr2	= hton32(0x00000000);

	mcast_all->addr.addr3	= hton32(0x00000000);
	mcast_all->addr.addr4	= hton32(0x00000001);

	//this one never expires
	mcast_all->expiration_date	= 0x0;
	mcast_all->scope		= MCAST_SCOPE;

	//TODO perform DAD
	
	//add them to the table of active addresses
	esix_add_to_active_addresses(ucast_ll);
	esix_add_to_active_addresses(mcast_ll);
	esix_add_to_active_addresses(mcast_all);

	//link local route (fe80::/64)
	struct esix_route_table_row *ll_rt	= MALLOC(sizeof(struct esix_route_table_row));
	
	ll_rt->addr.addr1	= hton32(0xfe800000);
	ll_rt->addr.addr2	= 0x0;
	ll_rt->addr.addr3	= 0x0;
	ll_rt->addr.addr4	= 0x0;
	ll_rt->mask		= 64;
	ll_rt->next_hop.addr1	= 0x0; //a value of 0 means no next hop 
	ll_rt->next_hop.addr1	= 0x0; 
	ll_rt->next_hop.addr1	= 0x0; 
	ll_rt->next_hop.addr1	= 0x0; 
	ll_rt->ttl		= DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	ll_rt->mtu		= intf_mtu;	
	ll_rt->expiration_date	= 0x0; //this never expires
	ll_rt->interface	= intf_index;

	//multicast route (ff00:: /8)
	struct esix_route_table_row *mcast_rt	= MALLOC(sizeof(struct esix_route_table_row));
	
	mcast_rt->addr.addr1		= hton32(0xff000000);
	mcast_rt->addr.addr2		= 0x0;
	mcast_rt->addr.addr3		= 0x0;
	mcast_rt->addr.addr4		= 0x0;
	mcast_rt->mask			= 8;
	mcast_rt->next_hop.addr1	= 0x0; //a value of 0 means no next hop 
	mcast_rt->next_hop.addr1	= 0x0; 
	mcast_rt->next_hop.addr1	= 0x0; 
	mcast_rt->next_hop.addr1	= 0x0; 
	mcast_rt->expiration_date	= 0x0; //this never expires
	mcast_rt->ttl			= DEFAULT_TTL;  // 1 should be ok, linux uses 255...
	mcast_rt->mtu			= intf_mtu;
	mcast_rt->interface		= intf_index;

	esix_add_to_active_routes(ll_rt);
	esix_add_to_active_routes(mcast_rt);
}

/**
 * esix_send_ttl_expired : sends a TTL expired message
 * back to its source.
 */
void	esix_send_ttl_expired(struct ip6_hdr *hdr)
{
}

/**
 * esix_send_router_sollicitation : crafts and sends a router sollicitation
 * on the interface specified by index. 
 */
void	esix_send_router_sollicitation(int intf_index)
{
}

/**
 * esix_parse_rtr_adv : parses RA messages, add/update a default route,
 * a prefix route and builds an IP address out of this prefix.
 */
void esix_parse_rtr_adv(struct icmp6_rtr_adv *rtr_adv, int length,
	 struct ip6_hdr *ip_hdr)
{
	int i=0;
	int j=0;
	struct icmp6_opt_prefix_info *pfx_info;
	struct icmp6_opt_mtu *mtu_info;
	struct icmp6_option_hdr *option_hdr;
	struct esix_route_table_row *default_rt	= NULL;
	struct esix_ipaddr_table_row *ucast_af;
	u8_t	*tmp;

	//we at least need to have 16 bytes to parse...
	//(router advertisement without any option = 16 bytes,
	//advertises only a default route)
	if(ntoh16(length) < 16 ) 
		return;

	//look up the routing table to see if this route already exists.
	//if it does, select it instead of creating a new one.
	

	while(i<ESIX_MAX_RT)
	{
		if((routes[i] != NULL) &&
			(routes[i]->addr.addr1		== 0x0) &&
			(routes[i]->addr.addr2		== 0x0) &&
			(routes[i]->addr.addr3		== 0x0) &&
			(routes[i]->addr.addr4		== 0x0) &&
			(routes[i]->next_hop.addr1	== ip_hdr->saddr1) &&
			(routes[i]->next_hop.addr2	== ip_hdr->saddr2) &&
			(routes[i]->next_hop.addr3	== ip_hdr->saddr3) &&
			(routes[i]->next_hop.addr4	== ip_hdr->saddr4))

			default_rt	= routes[i];
		i++;
	}

	if(default_rt == NULL)
	{
		default_rt = MALLOC(sizeof(struct esix_route_table_row));
		esix_add_to_active_routes(default_rt);  //add the pointer now to avoid adding it
							//twice when we're updating
	}
	//TODO : check if malloc succeeded

	
	default_rt->addr.addr1		= 0x0;
	default_rt->addr.addr2		= 0x0;
	default_rt->addr.addr3		= 0x0;
	default_rt->addr.addr4		= 0x0;
	default_rt->mask		= 0;
	default_rt->next_hop.addr1	= ip_hdr->saddr1;
	default_rt->next_hop.addr2	= ip_hdr->saddr2;
	default_rt->next_hop.addr3	= ip_hdr->saddr3;
	default_rt->next_hop.addr4	= ip_hdr->saddr4;
	default_rt->ttl			= rtr_adv->cur_hlim;
	default_rt->mtu			= DEFAULT_MTU;
	default_rt->expiration_date	= TIME + ntoh32(rtr_adv->rtr_lifetime);
	default_rt->interface		= INTERFACE;

	//parse options like MTU and prefix info
	i=16+2; 	//we at least need 2 more bytes (type + length) to be able to process
			//the first option field (those are TLVs)
	option_hdr = &rtr_adv->option_hdr;
	while (i < ntoh16(length)) // is the ip packet long enough to continue?
	{
		switch(ntoh16(option_hdr->type))
		{
			case PRFX_INFO:
				pfx_info = (struct icmp6_opt_prefix_info *) &option_hdr->payload; 
				if( (i+30) < ntoh16(length))
				{
					ucast_af = MALLOC (sizeof (struct esix_ipaddr_table_row)); 
					j=0;

					//first network bytes
					ucast_af->addr.addr1	= pfx_info->prefix.addr1;
					ucast_af->addr.addr2	= pfx_info->prefix.addr2;

					//host bytes
					ucast_af->addr.addr3	= 
						hton32(mac_addr[0] << 16 | tmp[1] << 8 | 0xff);

					ucast_af->addr.addr4	= 
						hton32(0xfe << 24 | tmp[0] << 16 | mac_addr[2]);
		
					//this one never expires
					ucast_af->expiration_date	= TIME + pfx_info->valid_lifetime;
					ucast_af->scope			= GLOBAL_SCOPE;

					while(j<ESIX_MAX_IPADDR)
					{
						if((addrs[j] != NULL) &&
							(addrs[j]->scope	== ucast_af->scope)	 &&
							(addrs[j]->expiration_date != 0) 		 &&
							(addrs[j]->addr.addr1	== ucast_af->addr.addr1) &&
							(addrs[j]->addr.addr2	== ucast_af->addr.addr2) &&
							(addrs[j]->addr.addr3	== ucast_af->addr.addr3) &&
							(addrs[j]->addr.addr4	== ucast_af->addr.addr4) &&
							(addrs[j]->mask		== 64))
						{
							//we're only updating
							addrs[j]->expiration_date = 
								ucast_af->expiration_date;
							FREE(ucast_af);
						}
						else
						//TODO perform DAD
						//try to add it to the table of addresses and
						//free it so we don't leak memory in case it fails
							if(!esix_add_to_active_addresses(ucast_af))
								FREE(ucast_af);
					}//while(j<...
				}//if(i+...
				i+=	30; 
				option_hdr	= (struct icmp6_option_hdr *)(((char*) rtr_adv)+i);
			break;

			case MTU:
				mtu_info = (struct icmp6_opt_mtu_info *) &rtr_adv->option_hdr.payload; 
				if( (i+6) < ntoh16(length))
				{
					default_rt->mtu	= ntoh16(mtu_info->mtu);
				}
				i+=8;
				option_hdr	= (struct icmp6_option_hdr *)(((char*) rtr_adv)+i);
			break;

			default:
				i+= (option_hdr->length*8) - 2; //option_hdr->length gives the size of
								//type + length + data fields in
								//8 bytes multiples
				option_hdr	= (struct icmp6_option_hdr *)(((char*) rtr_adv)+i);

			break; 	
		}
		i += 2; //try to read the next option header

	}
}