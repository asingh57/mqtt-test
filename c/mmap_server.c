#include <stdio.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/if_ether.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <unistd.h>

#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include <net/if.h>

#define PACKET_LENGTH 100


static char server_send_packet[PACKET_LENGTH]="\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x45\x00 server sent packet";
static int server_len= sizeof(server_send_packet);


static char client_str[PACKET_LENGTH]="client sent packet";
static int client_len= sizeof(client_str);
int custom_offset=17;//offset in frame header where this string starts

int setup_recv_buffer(struct pollfd *pollStruct){

    int mySocket = socket( AF_PACKET, SOCK_RAW, htons(ETH_P_IP) );
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, "lo");
    int err = ioctl(mySocket, SIOCGIFINDEX, &ifr);
    if (err < 0) {
		    perror("SIOCGIFINDEX");
		    exit(EXIT_FAILURE);
	    }
    return mySocket;
}


void receive_data(uint8_t *receive_buffer, struct pollfd *pollStruct,int mySocket){
        pollStruct[0].fd = mySocket;
        pollStruct[0].events = POLLIN;
        //pollStruct[0].revents = 0;

        if( poll(pollStruct, 1, 0) == 1)
        {
            //printf("New Frame Arrived: ");
            uint16_t len_recv = recvfrom( mySocket, receive_buffer, PACKET_LENGTH, 0, NULL, NULL);

            if(PACKET_LENGTH==len_recv){

                if(strncmp((receive_buffer+custom_offset),client_str,client_len)==0)

                    printf("string from packet received\n");
                }


        }
}



int setup_send_buffer(){
 

return 0;

}

int send_data(){


return 0;
}



int main()
{

    /* receive buffer setup*/
    uint8_t receive_buffer[1536];
    struct pollfd pollStruct[1];
    int receive_socket = setup_recv_buffer(pollStruct);
    //---------------------


    /* send buffer setup*/


    int send_socket;
    //-------------------

    while(1)
    {

        receive_data(receive_buffer,pollStruct,receive_socket);
        send_data();
        
    }
}
