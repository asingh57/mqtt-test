#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>



#ifndef likely
# define likely(x)		__builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
# define unlikely(x)		__builtin_expect(!!(x), 0)
#endif

 
#define PACKET_SZ 1024


/* params */
static char * str_devname= "lo";
static int c_packet_sz   = PACKET_SZ;
static int c_packet_nb   = 1;
static int c_buffer_sz   = PACKET_SZ*8;
static int c_buffer_nb   = 1;
static int c_sndbuf_sz   = 0;
static int c_mtu         = 0;
static int c_send_mask   = 127;
static int c_error       = 0;
static int mode_dgram    = 0;

static int mode_loss     = 0;
static int mode_verbose  = 0;
 
/* globals */
volatile int tx_fd_socket;
volatile int data_offset = 0;
volatile struct sockaddr_ll *ps_sockaddr = NULL;
volatile struct tpacket_hdr * tx_ps_header_start;
volatile int shutdown_flag = 0;
struct tpacket_req s_packet_req;

char tmp[PACKET_SZ]="\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x45\x00 server sent packet";



struct block_desc {
	uint32_t version;
	uint32_t offset_to_priv;
	struct tpacket_hdr_v1 h1;
};

struct ring {
	struct iovec *rd;
	uint8_t *map;
	struct tpacket_req3 req;
};

int read_offset=147;

char read_string[50]="client sent packet";

static unsigned long packets_total = 0, bytes_total = 0;



static int setup_socket_recv(struct ring *ring, char *netdev)
{
	int err, i, fd, v = TPACKET_V3;
	struct sockaddr_ll ll;
	unsigned int c_buffer_sz = 1024*8;
	unsigned int c_buffer_nb = 1;

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
		perror("socket");
		exit(1);
	}

	err = setsockopt(fd, SOL_PACKET, PACKET_VERSION, &v, sizeof(v));
	if (err < 0) {
		perror("setsockopt");
		exit(1);
	}

	memset(&ring->req, 0, sizeof(ring->req));
	ring->req.tp_block_size = c_buffer_sz;
	ring->req.tp_frame_size = c_buffer_sz;
	ring->req.tp_block_nr = c_buffer_nb;
	ring->req.tp_frame_nr = c_buffer_nb;
	ring->req.tp_retire_blk_tov = 60;
	ring->req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;

	err = setsockopt(fd, SOL_PACKET, PACKET_RX_RING, &ring->req,
			 sizeof(ring->req));
	if (err < 0) {
		perror("setsockopt");
		exit(1);
	}

	ring->map = mmap(NULL, ring->req.tp_block_size * ring->req.tp_block_nr,
			 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, 0);
	if (ring->map == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}




	ring->rd = malloc(ring->req.tp_block_nr * sizeof(*ring->rd));
	assert(ring->rd);
	for (i = 0; i < ring->req.tp_block_nr; ++i) {
		ring->rd[i].iov_base = ring->map + (i * ring->req.tp_block_size);
		ring->rd[i].iov_len = ring->req.tp_block_size;
	}

	memset(&ll, 0, sizeof(ll));
	ll.sll_family = PF_PACKET;
	ll.sll_protocol = htons(ETH_P_ALL);
	ll.sll_ifindex = if_nametoindex(netdev);
	ll.sll_hatype = 0;
	ll.sll_pkttype = 0;
	ll.sll_halen = 0;

	err = bind(fd, (struct sockaddr *) &ll, sizeof(ll));
	if (err < 0) {
		perror("bind");
		exit(1);
	}

	return fd;
}

static void display(struct tpacket3_hdr *ppd)
{
	struct ethhdr *eth = (struct ethhdr *) ((uint8_t *) ppd + ppd->tp_mac);
	struct iphdr *ip = (struct iphdr *) ((uint8_t *) eth + ETH_HLEN);

	if (eth->h_proto == htons(ETH_P_IP)) {
		struct sockaddr_in ss, sd;
		char sbuff[NI_MAXHOST], dbuff[NI_MAXHOST];

		memset(&ss, 0, sizeof(ss));
		ss.sin_family = PF_INET;
		ss.sin_addr.s_addr = ip->saddr;
		getnameinfo((struct sockaddr *) &ss, sizeof(ss),
			    sbuff, sizeof(sbuff), NULL, 0, NI_NUMERICHOST);

		memset(&sd, 0, sizeof(sd));
		sd.sin_family = PF_INET;
		sd.sin_addr.s_addr = ip->daddr;
		getnameinfo((struct sockaddr *) &sd, sizeof(sd),
			    dbuff, sizeof(dbuff), NULL, 0, NI_NUMERICHOST);

		printf("%s -> %s, ", sbuff, dbuff);
	}

	printf("rxhash: 0x%x\n", ppd->hv1.tp_rxhash);
}

static void walk_block(struct block_desc *pbd, const int block_num)
{
	int num_pkts = pbd->h1.num_pkts, i;
	unsigned long bytes = 0;
	struct tpacket3_hdr *ppd;

	ppd = (struct tpacket3_hdr *) ((uint8_t *) pbd +
				       pbd->h1.offset_to_first_pkt);
	for (i = 0; i < num_pkts; ++i) {
		bytes += ppd->tp_snaplen;
		display(ppd);

		ppd = (struct tpacket3_hdr *) ((uint8_t *) ppd +
					       ppd->tp_next_offset);
	}

	packets_total += num_pkts;
	bytes_total += bytes;
}

static void flush_block(struct block_desc *pbd)
{
	pbd->h1.block_status = TP_STATUS_KERNEL;
}

static void teardown_socket(struct ring *ring, int fd)
{
	munmap(ring->map, ring->req.tp_block_size * ring->req.tp_block_nr);
	free(ring->rd);
	close(fd);
}


int send_reply(){

    printf("eq\n");
    return 0;
}

/* This task will call send() procedure */
void *task_send(void *arg) {
	int ec_send;
	static int total=0;
	int blocking = (int) arg;
 
 
	do
	{
		/* send all buffers with TP_STATUS_SEND_REQUEST */
		/* Wait end of transfer */

		ec_send = sendto(tx_fd_socket,
				NULL,
				0,
				(blocking? 0 : MSG_DONTWAIT),
				(struct sockaddr *) ps_sockaddr,
				sizeof(struct sockaddr_ll));
 
		if(ec_send < 0) {
			perror("send");
			break;
		}
		else if ( ec_send == 0 ) {
			/* nothing to do => schedule : useful if no SMP */

            printf("aaa\n");
			usleep(0);
		}
		else {

			total += ec_send/(c_packet_sz);
			printf("send %d packets (+%d bytes)\n",total, ec_send);
			fflush(0);
            break;
		}
 
	} while(1==1);
 
	if(blocking) printf("end of task send()\n");
	//printf("end of task send(ec=%x)\n", ec_send);
 
	return (void*) ec_send;
}

/* This task will fill circular buffer */
void task_fill() {
	int i,j;
	int i_index = 0;
	char * data;
	int first_loop = 1;
	struct tpacket_hdr * ps_header;
	int ec_send = 0;
 
 
	for(i=1; i <= c_packet_nb; i++)
	{
		int i_index_start = i_index;
		int loop = 1;
 
		/* get free buffer */
		do {
			ps_header = ((struct tpacket_hdr *)((void *)tx_ps_header_start + (c_buffer_sz*i_index)));
			data = ((void*) ps_header) + data_offset;
			switch((volatile uint32_t)ps_header->tp_status)
			{
				case TP_STATUS_AVAILABLE:
					/* fill data in buffer */
					if(first_loop) {
						for(j=0;j<c_packet_sz;j++)
							data[j] = tmp[j];
					}
					loop = 0;
				break;
 
				case TP_STATUS_WRONG_FORMAT:
					printf("An error has occured during transfer\n");
					exit(EXIT_FAILURE);
				break;
 
				default:
					/* nothing to do => schedule : useful if no SMP */
					usleep(0);
					break;
			}
		}
		while(loop == 1);
 
		i_index ++;
		if(i_index >= c_buffer_nb)
		{
			i_index = 0;
			first_loop = 0;
		}
 
		/* update packet len */
		ps_header->tp_len = c_packet_sz;
		/* set header flag to USER (trigs xmit)*/
		ps_header->tp_status = TP_STATUS_SEND_REQUEST;
 
		/* if smp mode selected */

		
		/* send all packets */
		if( ((i&c_send_mask)==0) || (ec_send < 0) || (i == c_packet_nb) )
		{
			/* send all buffers with TP_STATUS_SEND_REQUEST */
			/* Don't wait end of transfer */
			ec_send = (int) task_send((void*)0);
		}
		
		else if(c_error) {
 
			if(i == (c_packet_nb/2))
			{
				int ec_close;
				if(mode_verbose) printf("close() start\n");
 
				if(c_error == 1) {
					ec_close = close(tx_fd_socket);
				}
				if(c_error == 2) {
					if (setsockopt(tx_fd_socket,
								 SOL_PACKET,
								 PACKET_TX_RING,
								 (char *)&s_packet_req,
								 sizeof(s_packet_req))<0)
					{
						perror("setsockopt: PACKET_TX_RING");
						//return EXIT_FAILURE;
					}
				}
				if(mode_verbose) printf("close end (ec:%d)\n",ec_close);
				break;
			}
		}
	}
	printf("end of task fill()\n");
}





int main(int argc, char **argp)
{

    tx_fd_socket = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(tx_fd_socket == -1)
	{
		perror("socket");
		return EXIT_FAILURE;
	}
    struct sockaddr_ll my_addr, peer_addr;
    memset(&my_addr, 0, sizeof(struct sockaddr_ll));
	my_addr.sll_family = PF_PACKET;
	my_addr.sll_protocol = htons(ETH_P_ALL);
    struct ifreq s_ifr;
    strncpy (s_ifr.ifr_name, str_devname, sizeof(s_ifr.ifr_name));
    int ec = ioctl(tx_fd_socket, SIOCGIFINDEX, &s_ifr);
	if(ec == -1)
	{
		perror("iotcl");
		return EXIT_FAILURE;
	}

    int i_ifindex = s_ifr.ifr_ifindex;

    memset(&my_addr, 0, sizeof(struct sockaddr_ll));
	my_addr.sll_family = AF_PACKET;
	my_addr.sll_protocol = ETH_P_ALL;
	my_addr.sll_ifindex = i_ifindex;

    /* bind port */
	if (bind(tx_fd_socket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_ll)) == -1)
	{
		perror("bind");
		return EXIT_FAILURE;
	}
	/* prepare Tx ring request */
	s_packet_req.tp_block_size = c_buffer_sz;
	s_packet_req.tp_frame_size = c_buffer_sz;
	s_packet_req.tp_block_nr = c_buffer_nb;
	s_packet_req.tp_frame_nr = c_buffer_nb;
 
 
	/* calculate memory to mmap in the kernel */
	int size = s_packet_req.tp_block_size * s_packet_req.tp_block_nr;
 
	/* set packet loss option */
	int t2 = mode_loss;
	if (setsockopt(tx_fd_socket,
								 SOL_PACKET,
								 PACKET_LOSS,
								 (char *)&t2,
								 sizeof(t2))<0)
	{
		perror("setsockopt: PACKET_LOSS");
		return EXIT_FAILURE;
	}
 
	/* send TX ring request */
	if (setsockopt(tx_fd_socket,
								 SOL_PACKET,
								 PACKET_TX_RING,
								 (char *)&s_packet_req,
								 sizeof(s_packet_req))<0)
	{
		perror("setsockopt: PACKET_TX_RING");
		return EXIT_FAILURE;
	}
 
 
	/* get data offset */
			data_offset = TPACKET_HDRLEN - sizeof(struct sockaddr_ll);
	printf("data offset = %d bytes\n", data_offset);
 
	/* mmap Tx ring buffers memory */
	tx_ps_header_start = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, tx_fd_socket, 0);
	if (tx_ps_header_start == (void*)-1)
	{
		perror("mmap");
		return EXIT_FAILURE;
	}


	int fd_recv, err;
	socklen_t len;
	struct ring ring;
	struct pollfd pfd;
	unsigned int block_num = 0, blocks = 1;
	struct block_desc *pbd;
	struct tpacket_stats_v3 stats;
	memset(&ring, 0, sizeof(ring));
	fd_recv = setup_socket_recv(&ring, "lo");
	assert(fd_recv > 0);

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd_recv;
	pfd.events = POLLIN | POLLERR;
	pfd.revents = 0;

	while (1==1) {
		pbd = (struct block_desc *) ring.rd[block_num].iov_base;

		if ((pbd->h1.block_status & TP_STATUS_USER) == 0) {
			poll(&pfd, 1, -1);
			continue;
		}
        if(strncmp(ring.map+read_offset,read_string,13)==0){
            task_fill();
                
        }
		flush_block(pbd);

	}

	teardown_socket(&ring, fd_recv);
	return 0;
}

