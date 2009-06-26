/**
 * @file
 * Standard API for UDP and TCP.
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
#include "tools.h"
#include "socket.h"
#include "include/socket.h"
#include "udp6.h"

u32_t socket(u16_t family, u8_t type, u8_t proto)
{	
	u32_t socket = 3; // TODO choose socket number and port
	u16_t port = 1024;
	
	if(family != AF_INET6)
		return -1;
	
	if(type == SOCK_DGRAM)
	{
		if(esix_socket_add(socket, SOCK_DGRAM, port) < 0)
			return -1;
	}
	return socket;
}

u32_t bind(u32_t socket, const struct sockaddr_in6 *address, u32_t len)
{
	return socket;
}

u32_t recvfrom(u32_t socket, void *buff, u16_t len, u8_t flags, struct sockaddr_in6* from, u32_t *fromaddrlen)
{
	int i;
	u32_t plen;
	struct udp_packet *packet;
	
	i = esix_socket_get_index(socket);
	if(i < 0)
		return 0; // TODO: error, how to check ? errno style ?
	
	if(sockets[i]->received == NULL)
	{
		plen = 0; // nothing received
	}
	else
	{
		packet = sockets[i]->received;
		plen = packet->len;
		if(plen <= len)
			esix_memcpy(buff, packet->data, plen);
		else
			esix_memcpy(buff, packet->data, len);
		
		from->sin6_port = packet->s_port;
		esix_memcpy(&from->sin6_addr, &packet->s_addr, 16);
		
		if(!(flags & MSG_PEEK))
		{
			esix_w_free(packet->data);
			esix_w_free(sockets[i]->received);
			sockets[i]->received = NULL;
		}
	}
	return plen;
}

u32_t sendto(u32_t socket, const void *buff, u16_t len, u8_t flags, const struct sockaddr_in6 *to, u32_t toaddrlen)
{
	int i = esix_socket_get_index(socket);
	
	esix_udp_send((struct ip6_addr *) &to->sin6_addr, sockets[i]->port, to->sin6_port, buff, len);
	
	return len;
}

int esix_socket_add_row(struct socket_table_row *row)
{
	int i;
	for(i = 0; i < ESIX_MAX_SOCK; i++)
	{
		if(sockets[i] == NULL)
		{
			sockets[i] = row;
			break;
		}
	}	
	if(i == ESIX_MAX_SOCK)
		return -1;
	return 0;
}

int esix_socket_add(u32_t socket, u8_t type, u16_t port)
{
	struct socket_table_row *row = esix_w_malloc(sizeof(struct socket_table_row));
	
	row->socket = socket;
	row->type = type;
	row->port = port;
	row->received = NULL;
	
	return esix_socket_add_row(row);
}

int esix_socket_get_index(u32_t socket)
{
	int j;
	for(j = 0; j<ESIX_MAX_SOCK; j++)
	{
		//check if we already stored this address
		if((sockets[j] != NULL) &&
			((sockets[j]->socket == socket)))
		{
			return j;
		}
	}
	return -1;
}

int esix_socket_get_port_index(u16_t port, u8_t type)
{
	int j;
	for(j = 0; j<ESIX_MAX_SOCK; j++)
	{
		//check if we already stored this address
		if((sockets[j] != NULL) &&
			(sockets[j]->port == port) &&
			(sockets[j]->type == type))
		{
			return j;
		}
	}
	return -1;
}