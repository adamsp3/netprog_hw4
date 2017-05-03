/* 
server.c
Names:  Hannah Deen   Perri Adams
RINs: 661135402   661176345
RCSIDs: deenh     adamsp3
Your task for this team-based (max. 2) assignment is to implement a file syncing utility similar in (reduced)
functionality to rsync.
  run:
    ./[executable] [server/client] [port number]
  ex: ./syncr server 12345
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
#include <sys/stat.h>
#include <stdlib.h>
 #include <sys/uio.h>


#define BUFFER_SIZE 255
#define FILEBUFF_SIZE 1024

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
    exit( EXIT_FAILURE );

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
    int sd = socket( PF_INET, SOCK_STREAM, 0 );
    /* sd is the socket descriptor */
    /*  and SOCK_STREAM == TCP       */

    if ( sd < 0 )
    {
      perror( "socket() failed" );
      exit( EXIT_FAILURE );
    }

    /* socket structures */
    struct sockaddr_in server;

    server.sin_family = PF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    /* htons() is host-to-network-short for marshalling */
    /* Internet is "big endian"; Intel is "little endian" */
    server.sin_port = htons( port );
    int len = sizeof( server );

    if ( bind( sd, (struct sockaddr *)&server, len ) < 0 )
    {
      perror( "bind() failed" );
      exit( EXIT_FAILURE );
    }

    listen( sd, 5 );   /* 5 is the max number of waiting clients */
    printf( "port %d\n", port );

    struct sockaddr_in client;
    int fromlen = sizeof( client );
    printf( "server address is %s\n", inet_ntoa( server.sin_addr ) );

    char buffer[BUFFER_SIZE];

    
    int newsock = accept(sd, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen);
    printf("Accepted client connection\n");

      /* handle new socket in a child process,
       allowing the parent process to immediately go
       back to the accept() call */
    

    /* Create the temporary directory */
    char template[] = "/tmp/tmpdir.XXXXXX";
    char *tmp_dirname = mkdtemp (template);

    if(tmp_dirname == NULL)
    {
    perror ("tempdir: error: Could not create tmp directory");
      exit (EXIT_FAILURE);
    }

    printf("%s\n", tmp_dirname );

    /* Change directory */
    if (chdir (tmp_dirname) == -1)
    {
      perror ("tempdir: error: ");
      exit (EXIT_FAILURE);
    }

    FILE *fp = NULL;

    fp = fopen(".4220_file_list.txt" ,"a+");

    if(fp == NULL)
    {
      perror ("fopen: error: ");
      exit (EXIT_FAILURE);
    }
    fprintf(fp, "b468ef8dfcc96cc15de74496447d7b45    Assignment4.pdf\n");
    fputs("d41d8cd98f00b204e9800998ecf8427e    foo.txt\n", fp);
    fclose(fp);

    int fd;
    struct stat file_stat;
    char file_size[256];
    int remain_data;
    int seg_size;
    int sent_bytes = 0;
    int offset;
    

    /* Receiving contents command */
    recv(newsock, buffer, BUFFER_SIZE, 0);

    if(strncmp(buffer,"contents",8)==0){

      fd = open(".4220_file_list.txt", O_RDONLY);
      if (fd == -1)
      {
        perror( "open() failed" );
        exit( EXIT_FAILURE );
      }

      /* Get file stats */
      if (fstat(fd, &file_stat) < 0)
      {
          perror( "fstat() failed" );
        exit( EXIT_FAILURE );
      }

      fprintf(stdout, "File Size: \n%lld bytes\n", file_stat.st_size);

      sprintf(file_size, "%lld", file_stat.st_size);

      /* Sending file size */
      len = send(newsock, file_size, sizeof(file_size), 0);
      if (len < 0)
      {
        perror( "send() failed" );
        exit( EXIT_FAILURE );
      }

      fprintf(stdout, "Server sent %d bytes for the size\n", len);

      offset = 0;
      remain_data = file_stat.st_size;

      /* Sending file data */
      int bytes_read = read(fd, buffer, sizeof(buffer));

      if (bytes_read < 0) 
      {
          perror( "read() failed" );
        exit( EXIT_FAILURE );
      }

      seg_size = BUFFER_SIZE;
    
      if(remain_data <= BUFFER_SIZE){
        seg_size = remain_data;
      }
    
      while ((remain_data > 0)&&((sent_bytes = send(newsock, buffer, seg_size, 0)) > 0))
      {
          fprintf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n %s", sent_bytes, offset, remain_data, buffer);
          remain_data -= sent_bytes;
          fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n %d", sent_bytes, offset, remain_data, seg_size);

          if(remain_data <= BUFFER_SIZE){
            seg_size = remain_data;
          }

          int bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0) // We're done reading from the file
            break;

        if (bytes_read < 0) 
        {
            perror( "read() failed" );
          exit( EXIT_FAILURE );
        }
      }
    }
    //fprintf(stdout, "Contents : %s\n", buffer);
}


void client(int port)
{
  /* create TCP client socket (endpoint) */
  int sock = socket( PF_INET, SOCK_STREAM, 0 );

    if ( sock < 0 )
    { 
      perror( "socket() failed" );
      exit( EXIT_FAILURE );
    }

    struct sockaddr_in server;
    server.sin_family = PF_INET;
    server.sin_port = htons( port );

    printf( "server address is %s\n", inet_ntoa( server.sin_addr ) );

    if(connect( sock, (struct sockaddr *)&server,
                sizeof( server ) ) < 0 )
    {
      perror( "connect() failed" );
      exit( EXIT_FAILURE );
    }

    char buffer[BUFFER_SIZE];
    char buffer2[BUFSIZ];

    int file_size;
    FILE *received_file;
    int remain_data = 0;
    ssize_t len;

     /*Contents*/
    int cont;
    char contents[] = "contents";
    cont = send(sock, contents, sizeof(contents), 0);
    if (cont < 0)
    {
      perror( "send() failed" );
      exit( EXIT_FAILURE );
    }

    /* Receiving file size */
    recv(sock, buffer2, BUFSIZ, 0);
    file_size = atoi(buffer2);
    fprintf(stdout, "File size : %d\n", file_size);

    received_file = fopen(".4220_file_list_serv.txt", "r+");
    if (received_file == NULL)
    {
      perror( "fopen() failed" );
      exit( EXIT_FAILURE );
    }
    //printf("hello\n");

    remain_data = file_size;
    int seg_size = BUFFER_SIZE;
    if(remain_data <= BUFFER_SIZE){
      seg_size = remain_data;
    }

    while ((remain_data > 0) &&((len = recv(sock, buffer, seg_size, 0)) > 0) && (remain_data > 0))
    {
        fwrite(buffer, sizeof(char), len, received_file);
        fprintf(stdout,"%s\n",buffer);
        remain_data -= len;
        if(remain_data <= BUFFER_SIZE){
          seg_size = remain_data;
        }
        fprintf(stdout, "Receive %zd bytes:- %d bytes\n", len, remain_data);
    }

    FILE *fp = fopen(".4220_file_list_serv.txt", "w+");
    fprintf(fp, "b468ef8dfcc96cc15de74496447d7b45    Assignment4.pdf\n");
    fputs("d41d8cd98f00b204e9800998ecf8425e    foo.txt\n", fp);
    fclose(fp);
    fp = fopen(".4220_file_list_serv.txt", "r+");
    if(fp){
      int ct = 0;
      rewind(received_file);
      char* s_MD5hash = calloc(BUFFER_SIZE,sizeof(char));
      char* s_fname = calloc(BUFFER_SIZE,sizeof(char));
      while(fscanf(received_file,"%s %s", s_MD5hash, s_fname)!=EOF){
        char* c_MD5hash = calloc(128,sizeof(char));
        char* c_fname = calloc(128,sizeof(char));
        rewind(fp);
        while(fscanf(fp,"%s %s", c_MD5hash, c_fname)!=EOF){
          if(strncmp(s_fname,c_fname,128)){
            if(strncmp(s_MD5hash,c_MD5hash,128)){
              fprintf(stdout, "hash match\n" );
            } 
            else{
              fprintf(stdout, "hash no match\n");
            }
          }
          else{
            fprintf(stdout, "nothing matches\n");
          }
        }
      }
    }

  close( sock );

}



