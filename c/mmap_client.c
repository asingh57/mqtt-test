#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "./benchmark.h"


#define FILEPATH "/tmp/mmapbenchmark.bin"
#define MESSAGE_CONTENT "hello server\n"
#define MESSAGE_SIZE 100
#define CONNECTION_MADE_STR "connection_made\n"
#define RECV_CONTENT "hello client\n"



int fd;
int *map_ptr;  /* pointer to memory space of mmap */
int res;

char *shared_message;
char client_message[MESSAGE_SIZE];

int create_connection(){
    fd = open(FILEPATH, O_RDWR);
    if (fd == -1) {
	perror("Error opening file for reading");
	exit(EXIT_FAILURE);
    }

    map_ptr = mmap(0, sizeof(client_message), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_ptr == MAP_FAILED) {
	close(fd);
	perror("Error mmapping the file");
	exit(EXIT_FAILURE);
    }

    shared_message = map_ptr;
    
}

int close_connection(){

    if (munmap(map_ptr, sizeof(client_message)) == -1) {
	    perror("Error un-mmapping the file");
	/* Decide here whether to close(fd) and exit() or not. Depends... */
    }

    /* Un-mmaping doesn't close the file, so we still need to do that.
     */
    close(fd);
    int status = remove(FILEPATH);
    printf("%d\n", status);
}

int send_message(){
    strncpy(shared_message, &client_message, MESSAGE_SIZE);
}

int wait_reply(){
    while(strcmp(RECV_CONTENT, shared_message)!=0){
            //busy wait
    }
    
}


int main(int argc, char *argv[])
{
    create_connection();
    strncpy(client_message, MESSAGE_CONTENT, MESSAGE_SIZE);

    if(strcmp(CONNECTION_MADE_STR, shared_message)!=0){
        printf("%s %s %c\n","server not available",shared_message);
        close_connection();
        exit(-1);
    }

    for (unsigned long long i = 0; i < BENCHMARK_ITERATIONS; i++) {
        start_clock();
        send_message();
        wait_reply();
        stop_clock();
    }
    
    print_result(0);
    
    close_connection();

    return 0;
}
