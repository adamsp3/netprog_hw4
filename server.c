/* 

server.c

Names:	Hannah Deen		Perri Adams

RINs:	661135402		661176345

RCSIDs:	deenh			adamsp3

Your task for this team-based (max. 2) assignment is to implement a file syncing utility similar in (reduced)
functionality to rsync.

	run:
		./[executable] [server/client] [port number]
	ex:	./syncr server 12345
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdarg.h>
#include <assert.h>
#include <fcntl.h>

#define PAYLOAD_SIZE 1016
#define HEADER_SIZE 5
#define ACK 1
#define FIN 2
#define PACKET_SIZE 1024

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char**argv)
{

    int sockfd, numbytes, local_port;  // listen on sock_fd, new connection on new_fd
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    char s[INET6_ADDRSTRLEN];

    //creating the socket
    struct sockaddr_in servaddr, temp_addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //getting random port assigment
    servaddr.sin_port = htons(0);

    //binding port to sokect
    bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    socklen_t sz = sizeof(temp_addr);

    int ret = getsockname(sockfd, (struct sockaddr*)&temp_addr, &sz);
    if (ret < 0) {
        printf("Problem!\n");
        exit(-1);
    }
    char buffer[1028];
    //converting to the correct byte order for register
    local_port = ntohs(temp_addr.sin_port);

   
        
    //specified in the assigment
    printf("Port number: %d\n", local_port);

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
    	
        sin_size = sizeof their_addr;
        numbytes = recvfrom(sockfd, buffer,  1028, 0, (struct sockaddr *)&their_addr, &sin_size);
        printf("%d\n", numbytes );
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
        
        //check_op(buffer);



        //close(new_fd);  // parent doesn't need this
    }
    return 0;
}



