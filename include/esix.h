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

#ifndef _ESIX_API_H
#define _ESIX_API_H

/**
 * Error type
 */
typedef enum {
	esix_err_none = 0, // went okay
	esix_err_failed,   // function failed
	esix_err_badparam, // bad parameter
	esix_err_oom       // out of memory
} esix_err;

/**
 * Sets up the esix stack.
 *
 * @param lla           MAC address of the node, string with each byte in hex separated by ':'.
 * @param send_callback A callback sending data of len bytes to the ethernet controller
 *
 * Usage:
 * @code
 * 	esix_init("XX:XX:XX:XX:XX:XX", my_send_callback);
 * @endcode
 */
void esix_init(char *lla, void (*send_callback)(void *data, int len));

/*
 * Process a received IPv6 packet.
 * 
 * @param packet is a pointer to the packet.
 * @param len is the packet size.
 */
void esix_eth_process(const void *payload, int len);

/*
 * ipv6 stack clock signal.
 *
 * Needs to be called every second by the user.
 *
 */
void esix_periodic_callback();

#endif
