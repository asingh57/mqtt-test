#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <poll.h>
#include <pthread.h>
 
/* params */
static char * str_devname= "lo";
static int c_packet_sz   = 150;
static int c_packet_nb   = 1000;
static int c_buffer_sz   = 1024*8;
static int c_buffer_nb   = 1024;
static int c_sndbuf_sz   = 0;
static int c_mtu         = 0;
static int c_send_mask   = 127;
static int c_error       = 0;
static int mode_dgram    = 0;
static int mode_thread   = 0;
static int mode_loss     = 0;
static int mode_verbose  = 0;
 
int i_ifindex;
/* globals */
volatile int fd_socket;
volatile int data_offset = 0;
volatile struct sockaddr_ll *ps_sockaddr = NULL;
volatile struct tpacket_hdr * ps_header_start;
volatile int shutdown_flag = 0;
struct tpacket_req s_packet_req;
 
void task_send(){

    char buff[100]= "aaaaaaaaaaaaaaaaa\n";
    int ec_send = send(fd_socket,
				buff,
				100,
				0);

    if(ec_send!=-1){
    printf("ec_send %d\n",ec_send);
    }

}


int main(){

    int mode_socket;
    struct ifreq s_ifr;
	struct sockaddr_ll my_addr, peer_addr;

    if (mode_dgram) {
		mode_socket = SOCK_DGRAM;
	}
	else
		mode_socket = SOCK_RAW;
 
	fd_socket = socket(PF_PACKET, mode_socket, htons(ETH_P_ALL));
    


	if(fd_socket == -1)
	{
		perror("socket");
		return EXIT_FAILURE;
	}

/*

    char dstaddr[ETH_ALEN] = {0x00,0x00,0x00,0x00,0x00,0x00};
		peer_addr.sll_family = AF_PACKET;
		peer_addr.sll_protocol = htons(ETH_P_IP);
		peer_addr.sll_ifindex = i_ifindex;
		peer_addr.sll_halen = ETH_ALEN;
		memcpy(&peer_addr.sll_addr, dstaddr, ETH_ALEN);
		ps_sockaddr = &peer_addr;
*/

    memset(&my_addr, 0, sizeof(struct sockaddr_ll));
	my_addr.sll_family = PF_PACKET;
	my_addr.sll_protocol = htons(ETH_P_ALL);

    /* initialize interface struct */
	strncpy (s_ifr.ifr_name, str_devname, sizeof(s_ifr.ifr_name));
 
	/* Get the broad cast address */
	int ec = ioctl(fd_socket, SIOCGIFINDEX, &s_ifr);


    if(ec == -1)
	{
		perror("iotcl");
		return EXIT_FAILURE;
	}
	/* update with interface index */
	i_ifindex = s_ifr.ifr_ifindex;
 
	/* new mtu value */
	if(c_mtu) {
		s_ifr.ifr_mtu = c_mtu;
		/* update the mtu through ioctl */
		ec = ioctl(fd_socket, SIOCSIFMTU, &s_ifr);
		if(ec == -1)
		{
			perror("iotcl");
		return EXIT_FAILURE;
		}
	}
 
	/* set sockaddr info */
	memset(&my_addr, 0, sizeof(struct sockaddr_ll));
    my_addr.sll_family = AF_PACKET;
	my_addr.sll_protocol = ETH_P_ALL;
	my_addr.sll_ifindex = i_ifindex;


    
	/* bind port */
	if (bind(fd_socket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_ll)) == -1)
	{
		perror("bind");
		return EXIT_FAILURE;
	}

    /* prepare Tx ring request */
    struct tpacket_req s_packet_req;
	s_packet_req.tp_block_size = c_buffer_sz;
	s_packet_req.tp_frame_size = c_buffer_sz;
	s_packet_req.tp_block_nr = c_buffer_nb;
	s_packet_req.tp_frame_nr = c_buffer_nb;


    /* calculate memory to mmap in the kernel */
	uint32_t size = s_packet_req.tp_block_size * s_packet_req.tp_block_nr;
 
	/* set packet loss option */
	int tmp = mode_loss;
	if (setsockopt(fd_socket,
								 SOL_PACKET,
								 PACKET_LOSS,
								 (char *)&tmp,
								 sizeof(tmp))<0)
	{
		perror("setsockopt: PACKET_LOSS");
		return EXIT_FAILURE;
	}
 
	/* send TX ring request */
	if (setsockopt(fd_socket,
								 SOL_PACKET,
								 PACKET_TX_RING,
								 (char *)&s_packet_req,
								 sizeof(s_packet_req))<0)
	{
		perror("setsockopt: PACKET_TX_RING");
		return EXIT_FAILURE;
	}
 
 
	/* change send buffer size */
	if(c_sndbuf_sz) {
		printf("send buff size = %d\n", c_sndbuf_sz);
		if (setsockopt(fd_socket, SOL_SOCKET, SO_SNDBUF, &c_sndbuf_sz,
					sizeof(c_sndbuf_sz))< 0)
		{
			perror("getsockopt: SO_SNDBUF");
			return EXIT_FAILURE;
		}
	}
 
	/* get data offset */
			data_offset = TPACKET_HDRLEN - sizeof(struct sockaddr_ll);
	printf("data offset = %d bytes\n", data_offset);
 
	/* mmap Tx ring buffers memory */
	ps_header_start = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_socket, 0);
	if (ps_header_start == (void*)-1)
	{
		perror("mmap");
		return EXIT_FAILURE;
	}






    while(1==1){
        task_send();

    }

}





