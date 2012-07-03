#ifndef _SOCKET_H
#define _SOCKET_H

#include "include/socket.h"
#include "ip6.h"

#define ESIX_MAX_SOCK 65536

enum state
{
	CLOSED,
	LISTEN,
	SYN_SENT,
	SYN_RECEIVED,
	ESTABLISHED,
	FIN_WAIT_1,
	FIN_WAIT_2,
	CLOSE_WAIT,
	CLOSING,
	LAST_ACK,
	TIME_WAIT,
	RESERVED //internal state
};

enum direction
{
	IN,
	OUT
};

enum action
{
	KEEP,
	EVICT
};

//queue element type
enum qe_type
{
	CHILD_SOCK, //child socket, created upon SYN reception
	SENT_PKT,
	RECV_PKT
};

struct sock_queue
{
	enum qe_type qe_type;
	int socknum; 			//only used with CHILD_SOCK
	struct sockaddr_in6 *sockaddr;  //only used by UDP for RX packets, addr&port of sender
	uint32_t seqn;			//stores the sequence number of this packet
	void *data; //actual data
	int data_len; //data length
	uint32_t t_sent; //time at which the packet was queued
	struct sock_queue *next_e; //next queued element
};

//actual session sockets, holding seq/ack/etc info
struct esix_sock
{
	uint8_t proto;
	volatile enum state state;
	esix_ip6_addr laddr;
	esix_ip6_addr raddr;
	uint16_t lport;
	uint16_t rport;
	uint32_t seqn;
	uint32_t ackn;
	uint32_t rexmit_date; //date at which to trigger retransmission
	struct sock_queue *queue; //stores sent/recvd data
};

#define FIND_ANY 0
#define FIND_CONNECTED 1
#define FIND_LISTEN 2

struct esix_sock esix_sockets[ESIX_MAX_SOCK];
uint16_t esix_last_port;

int esix_port_available(const uint16_t);
int esix_socket_create_child(const esix_ip6_addr *, const esix_ip6_addr *, uint16_t, uint16_t, uint8_t);
int esix_find_socket(const esix_ip6_addr *, const esix_ip6_addr *, uint16_t, uint16_t, uint8_t, uint8_t);
int esix_queue_data(int, const void *, int, struct sockaddr_in6 *, enum direction);
struct sock_queue * esix_socket_find_e(int , enum qe_type, enum action);
void esix_socket_init();
void esix_socket_free_queue(int);
int esix_socket_expire_e(int, uint32_t);
void esix_socket_housekeep();
#endif
