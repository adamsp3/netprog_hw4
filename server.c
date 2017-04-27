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

#define BUFFER_SIZE 1024

void die(const char*);

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void server(int port);
void client(int port);

int main(int argc, char**argv)
{
	if(argc < 3)
	{
		printf("too few arguments\n");
		exit(EXIT_FAILURE);

	}
  
	if(strcmp(argv[1], "server") == 0)
	{
		server(atoi(argv[2]));
	}
	else if(strcmp(argv[1], "client") == 0)
	{
		client(atoi(argv[2]));
	}
	else
	{
		printf("inputs must be either \"server\" or \"client\" \n");
	}
    
  return 0;
}



void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void server(int port)
{
	/* Create the listener socket as TCP socket */
  	int sd = socket(PF_INET, SOCK_STREAM, 0);
  	/* sd is the socket descriptor */
  	/*  and SOCK_STREAM == TCP       */

  	if (sd < 0)
  	{
    	perror("socket() failed");
    	exit(EXIT_FAILURE);
  	}

  	/* socket structures */
  	struct sockaddr_in server;

  	server.sin_family = PF_INET;
  	server.sin_addr.s_addr = INADDR_ANY;

  	/* htons() is host-to-network-short for marshalling */
  	/* Internet is "big endian"; Intel is "little endian" */
  	server.sin_port = htons(port);
  	int len = sizeof(server);

  	if (bind(sd, (struct sockaddr *)&server, len) < 0)
  	{
    	perror("bind() failed");
    	exit(EXIT_FAILURE);
  	}

  	listen(sd, 5);   /* 5 is the max number of waiting clients */
  	printf("PARENT: Listener bound to port %d\n", port);

  	struct sockaddr_in client;
  	int fromlen = sizeof(client);
  	printf("server address is %s\n", inet_ntoa(server.sin_addr));

  	char buffer[ BUFFER_SIZE ];

  	while (1)
  	{
    	int newsock = accept(sd, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen);
    	printf("PARENT: Accepted client connection\n");

    	/* handle new socket in a child process,
       allowing the parent process to immediately go
       back to the accept() call */
    	}

}


void client(int port)
{
	/* create TCP client socket (endpoint) */
	int sock = socket(PF_INET, SOCK_STREAM, 0);

  	if (sock < 0)
  	{	
    	perror("socket() failed");
    	exit(EXIT_FAILURE);
  	}

  	struct hostent * hp = gethostbyname("0.0.0.0");
  	if (hp == NULL)
  	{
    	perror("gethostbyname() failed");
    	exit(EXIT_FAILURE);
  	}

  	struct sockaddr_in server;
  	server.sin_family = PF_INET;
  	memcpy((void *)&server.sin_addr, (void *)hp->h_addr,
          hp->h_length);
  	server.sin_port = htons(port);

  	printf("server address is %s\n", inet_ntoa(server.sin_addr));

  	if (connect(sock, (struct sockaddr *)&server,
                sizeof(server)) < 0)
  	{
    	perror("connect() failed");
    	exit(EXIT_FAILURE);
  	}

  	char * msg = "hello world";
  	int n = write(sock, msg, strlen(msg));
	  fflush(NULL);
  	if (n < strlen(msg))
  	{
    	perror("write() failed");
    	exit(EXIT_FAILURE);
  	}

  	char buffer[ BUFFER_SIZE ];
  	n = read(sock, buffer, BUFFER_SIZE);  // BLOCK
  	if (n < 0)
	  {
    	perror("read() failed");
	    exit(EXIT_FAILURE);
  	}
  	else
  	{
    	buffer[n] = '\0';
    	printf("Received message from server: %s\n", buffer);
  	}

  close(sock);

}



