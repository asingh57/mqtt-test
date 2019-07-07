#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "./benchmark.h"


#define FILEPATH "/tmp/mmapbenchmark.bin"
#define MESSAGE_CONTENT "hello client\n"
#define MESSAGE_SIZE 100
#define CONNECTION_MADE_STR "connection_made\n"
#define RECV_CONTENT "hello server\n"



int fd;
int *map_ptr;  /* pointer to memory space of mmap */
int res;


char server_message[MESSAGE_SIZE];
char *shared_message;//message sent and received here




int create_connection(){
    fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
	    perror("Error opening file for writing");
	    exit(EXIT_FAILURE);
    }
    res = lseek(fd, sizeof(server_message)-1, SEEK_SET);//stretch file to struct size
    if (res == -1) {
	    close(fd);
	    perror("Error calling lseek() to 'stretch' the file");
	    exit(EXIT_FAILURE);
    }
    res = write(fd, "", 1);//write at the end to set new file size
    if (res != 1) {
	    close(fd);
	    perror("Error writing last byte of the file");
	    exit(EXIT_FAILURE);
    }
    map_ptr = mmap(0, sizeof(server_message), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
	    close(fd);
	    perror("Error mmapping the file");
	    exit(EXIT_FAILURE);
    }

    shared_message = map_ptr;
    
}

int close_connection(){

    if (munmap(map_ptr, sizeof(server_message)) == -1) {
	    perror("Error un-mmapping the file");
	/* Decide here whether to close(fd) and exit() or not. Depends... */
    }

    /* Un-mmaping doesn't close the file, so we still need to do that.
     */
    close(fd);
}


int send_message(){
    strncpy(shared_message, &server_message, MESSAGE_SIZE);
}

int wait_reply(){
    while(strcmp(RECV_CONTENT, shared_message)!=0){
            //busy wait
    }
}


int main(int argc, char *argv[])
{
    strncpy(server_message, MESSAGE_CONTENT, MESSAGE_SIZE);    

    create_connection();//opens file
    strncpy(shared_message, CONNECTION_MADE_STR, MESSAGE_SIZE);//make it known to client that server is ready to take up connection

    for (unsigned long long i = 0; i < BENCHMARK_ITERATIONS; i++) {
         
        wait_reply();
        send_message();
    }

    
    close_connection();//closes the file
    
    return 0;
}
