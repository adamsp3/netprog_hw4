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
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>


#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#define BUFFER_SIZE 512

void die(const char*);
int send_file(char* file, char* buffer, int newsock);
int recv_file(char* file, char* buffer, int sock);

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

    server.sin_port = htons( port );
    int len = sizeof( server );

    if (bind( sd, (struct sockaddr *)&server, len ) < 0){
      perror("bind() failed");
      exit( EXIT_FAILURE );
    }

    listen(sd, 5);   /* 5 is the max number of waiting clients */
    printf( "port %d\n", port );

    struct sockaddr_in client;
    int fromlen = sizeof( client );
    printf("server address is %s\n", inet_ntoa(server.sin_addr));

    char buffer[BUFFER_SIZE];

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
    if (chdir (tmp_dirname) == -1){
      perror ("tempdir: error: ");
      exit (EXIT_FAILURE);
    }

    FILE *fp = NULL;

    fp = fopen(".4220_file_list.txt" ,"a+");

    if(fp == NULL){
      perror ("fopen: error: ");
      exit (EXIT_FAILURE);
    }

    while(1){    
      int newsock = accept(sd, (struct sockaddr *)&client,
                            (socklen_t*)&fromlen);
      printf("Accepted client connection\n");

      /* handle new socket in a child process,
      allowing the parent process to immediately go
      back to the accept() call */
      

      /* Receiving contents command */
      int n = recv(newsock, buffer, BUFFER_SIZE,0);
      if(n < 0){
        perror( "recv() failed\n");
      }

      else if(n >= 0 && strncmp(buffer,"contents",8)==0){

        /*Sends the file list to the client*/
        send_file(".4220_file_list.txt", buffer, newsock);

        
        char req_buff[BUFFER_SIZE];
        char garbage[BUFFER_SIZE];
        char file[BUFFER_SIZE];
        char hash[BUFFER_SIZE];
        char hash2[BUFFER_SIZE];
        char buff2[BUFFER_SIZE];

        int rqsz;

        /*Gets number of file requests (query, pull, get) */
        int n = recv(newsock, buffer, BUFFER_SIZE,0);
        if ( n < 0 ){
          perror( "recv() failed\n");
          exit(EXIT_FAILURE);
        }

        rqsz = atoi(buffer);
        //fprintf(stdout, "File size : %d\n", rqsz);

        fprintf(stdout, "Number of file transfers: %d\n", rqsz);
        /*Receives requests*/
        int i = 0;
        for(; i < rqsz; i++){
          /*Receive request*/
          n = recv(newsock, req_buff, BUFFER_SIZE, 0);
          if ( n < 0 ){
            perror( "recv() failed\n");
            exit(EXIT_FAILURE);
          }

          /*get request*/
          if(strncmp(req_buff,"get",3)==0){
            sscanf (req_buff,"%s %s %s", garbage, hash, file);
            send_file(file, buff2,newsock);
          }
          
          /*put request*/
          else if(strncmp(req_buff,"put",3)==0){
            sscanf (req_buff,"%s %s %s", garbage, hash, file);
            recv_file(file, buff2, newsock);
          }
          
          /*query request*/
          else if(strncmp(req_buff,"query",5)==0){
            sscanf (req_buff,"%s %s %s %s", garbage,hash, hash2, file);

            /*Get time of server file*/
            struct stat *file_info = malloc(sizeof(struct stat));
            if (lstat(file, file_info) != 0) {
              perror("lstat() error");
              exit(EXIT_FAILURE);
            }
            char date[BUFFER_SIZE];
            time_t val = file_info->st_mtime;
            strftime(date, BUFFER_SIZE, "%Y %m %d %H %M %S", localtime(&val));
            
            /*Send time to client*/
            n = send(newsock, date, BUFFER_SIZE, 0);
            if ( n < 0 ){
              perror( "send() failed\n");
              exit(EXIT_FAILURE);
            }

            /*Receive request based on which file was newer*/
            n = recv(newsock, req_buff, BUFFER_SIZE, 0);
            if ( n < 0 ){
              perror( "recv() failed\n");
              exit(EXIT_FAILURE);
            }

            /*get request if server file was newer*/
            if(strncmp(req_buff,"get",3)==0){
              sscanf (req_buff,"%s %s %s", garbage, hash, file);
              send_file(file, buff2,newsock);
            }
            
            /*pur request if client file was newer*/
            else if(strncmp(req_buff,"put",3)==0){
              sscanf (req_buff,"%s %s %s", garbage, hash, file);
              recv_file(file, buff2, newsock);
            }
          }
        }

        /*Generate new .4220_file_list.txt*/
        FILE *fp = fopen(".4220_file_list.txt", "w+");
        DIR * dirp = opendir(tmp_dirname);
        struct dirent * dp;
        while ((dp = readdir(dirp)) != NULL){
          unsigned char c[MD5_DIGEST_LENGTH];
          char *filename = dp->d_name;
          int i;

          /*Only filenames that begin with a number or letter are included*/
          if((filename[0] > 47 &&filename[0]<58)||(filename[0] > 64 &&filename[0]<91) ||(filename[0] > 96 &&filename[0]<122)){
            FILE *inFile = fopen (filename, "rb");
            MD5_CTX mdContext;
            int bytes;
            unsigned char data[1024];
            if (inFile == NULL) {
                printf ("%s can't be opened.\n", filename);
                continue;
            }
            MD5_Init (&mdContext);
            while ((bytes = fread (data, 1, 1024, inFile)) != 0)
                MD5_Update (&mdContext, data, bytes);
            MD5_Final (c,&mdContext);

            for(i = 0; i < MD5_DIGEST_LENGTH; i++) {fprintf(fp,"%02x", c[i]);}
            fprintf (fp,"%s %s\n", "   ",filename);
            fclose (inFile);
          }
         }
         (void)closedir(dirp);
         fclose(fp);

      /*End this connection*/
      }
      close(newsock);
    }

    /*Close tmp directory*/
    char rm_command[26];

    strncpy (rm_command, "rm -rf ", 7 + 1);
    strncat (rm_command, tmp_dirname, strlen (tmp_dirname) + 1);

    if (system (rm_command) == -1)
    {
       perror ("tempdir: error: ");
       exit (EXIT_FAILURE);
    }

    /*Close socket*/
    close(sd);
}


void client(int port){ 

 /*Generate a file list based on the current directory*/
  FILE *fp = fopen(".4220_file_list.txt", "w+");
  DIR * dirp = opendir(".");
  struct dirent * dp;
  while ((dp = readdir(dirp)) != NULL){
    unsigned char c[MD5_DIGEST_LENGTH];
    char *filename = dp->d_name;
    int i;
    if((filename[0] > 47 &&filename[0]<58)||(filename[0] > 64 &&filename[0]<91) ||(filename[0] > 96 &&filename[0]<122)){
      FILE *inFile = fopen (filename, "rb");
      MD5_CTX mdContext;
      int bytes;
      unsigned char data[1024];
      if (inFile == NULL) {
          continue;
      }
      MD5_Init (&mdContext);
      while ((bytes = fread (data, 1, 1024, inFile)) != 0)
          MD5_Update (&mdContext, data, bytes);
      MD5_Final (c,&mdContext);

      for(i = 0; i < MD5_DIGEST_LENGTH; i++) {fprintf(fp,"%02x", c[i]);}
      fprintf (fp,"%s %s\n", "   ",filename);
      fclose (inFile);
    }
 }
 (void)closedir(dirp);

  /* create TCP client socket (endpoint) */
  int sock = socket( PF_INET, SOCK_STREAM, 0 );
  int n;
    if ( sock < 0 )
    { 
      perror( "socket() failed" );
      exit( EXIT_FAILURE );
    }

    struct sockaddr_in server;
    server.sin_family = PF_INET;
    server.sin_port = htons( port );

    printf( "server address is %s\n", inet_ntoa(server.sin_addr));

    if(connect( sock, (struct sockaddr *)&server,
                sizeof( server ) ) < 0 ){
      perror( "connect() failed" );
      exit( EXIT_FAILURE );
    }

    char buffer[BUFFER_SIZE];
    char buffer2[BUFFER_SIZE];

    int file_size;
    int remain_data = 0;
    ssize_t len;

     /*Calls Contents command*/
    int cont;
    char contents[BUFFER_SIZE];
    sprintf(contents,"%s", "contents");
    cont = send(sock, contents, sizeof(contents),0);
    fflush(NULL);
    if (cont < 0){
      perror( "send() failed" );
      exit( EXIT_FAILURE );
    }

    /*recieves .4220_file_list_serv.txt from server*/
    recv_file(".4220_file_list_serv.txt", buffer, sock);

    /*
    received_file is the file list sent by the server.
    matches will hold the list of client files that names match
      files on the server list.
    requests will hold the list of requests to be sent to the server.
      These are compiled beforehand so the server can know how many to expect.
    */
    FILE *received_file = fopen(".4220_file_list_serv.txt", "r");
    FILE *matches = fopen(".4220_file_list_matches.txt", "w+");
    FILE *requests = fopen(".requests.txt", "w");
    fp = fopen(".4220_file_list.txt", "r+");

    /*Count the number of requests*/
    int count = 0;
    if(fp){
      int ct = 0;
      rewind(received_file);

      /*Hold the names and hashes parsed from the file list*/
      char s_MD5hash[BUFFER_SIZE];
      char s_fname[BUFFER_SIZE];
      char c_MD5hash[BUFFER_SIZE];
      char c_fname[BUFFER_SIZE];
      while(fscanf(received_file,"%s %s", s_MD5hash, s_fname)!=EOF){
        rewind(fp);

        while(fscanf(fp,"%s %s", c_MD5hash, c_fname)!=EOF){
          /*Checks if names match*/
          if(strncmp(s_fname,c_fname,128)==0){
            /*Checks if hashes match*/
            if(strncmp(s_MD5hash,c_MD5hash,128)==0){
              /*Prints them to matches*/
              fprintf(matches,"%s %s\n",s_MD5hash, s_fname);
            } 
            else{
              /*If names match,print them to matches and add a query request to requests*/
              fprintf(matches, "%s %s\n",s_MD5hash, s_fname);
              fprintf(requests, "%s %s %s %s\n", "query", s_MD5hash, c_MD5hash, c_fname);
              /*Increment count of requests*/
              count++;
            }
            break;
          }
        }

        if(strncmp(s_fname,c_fname,128)!=0){
          /*If a file on the server cannot be found on the client, add get request to requests*/
          fprintf(requests, "%s %s %s\n", "get", s_MD5hash, s_fname);
          count++;
        }
      }

      rewind(fp);

      /*Find the files on the client that don't match files on the server*/
      while(fscanf(fp,"%s %s", c_MD5hash, c_fname)!=EOF){
        char m_MD5hash[BUFFER_SIZE];
        char m_fname[BUFFER_SIZE];
        rewind(matches);
        while(fscanf(matches,"%s %s", m_MD5hash, m_fname)!=EOF){
          if(strncmp(m_fname,c_fname,128)==0){
            break;
          }
        }
        if(strncmp(m_fname,c_fname,128)!=0){
          /*If a file on the client cannot be found on the server, add put request to requests*/
          fprintf(requests,"%s %s %s\n", "put", c_MD5hash, c_fname); 
          count++;
        }
      }
    }

    fclose(received_file);
    fclose(matches);
    fclose(fp);
    fclose(requests);


    char sendbuff[BUFFER_SIZE];

    sprintf(sendbuff, "%d", count);

    /*Send the number of requests*/
    n = send(sock, sendbuff, BUFFER_SIZE, 0);
    fflush(NULL);
    if (n < 0){
      perror( "send() failed" );
      exit( EXIT_FAILURE );
    }
    fprintf(stdout, "Number of file transfers: %d\n", count);

    requests = fopen(".requests.txt", "r");
    char garbage[BUFFER_SIZE];
    char file[BUFFER_SIZE];
    char hash[BUFFER_SIZE];
    char hash2[BUFFER_SIZE];
    int i = 0;

    rewind(requests);

    /*Send requests*/
    for(; i < count; i++){
      if(fgets(sendbuff, BUFFER_SIZE,requests) != NULL){
        fprintf(stdout,"Request: %s", sendbuff);
        
        n = send(sock, sendbuff, BUFFER_SIZE, 0); 
        if (n < 0){
          perror("send() failed\n");
          exit(EXIT_FAILURE);
        }

        /*Receive file after sending a get request*/
        if(strncmp(sendbuff,"get",3)==0){
          sscanf (sendbuff,"%s %s %s", garbage, hash, file);
          recv_file(file, buffer,sock);

        }
        
        /*Send file after sending a put request*/
        else if(strncmp(sendbuff,"put",3)==0){
          sscanf(sendbuff,"%s %s %s", garbage,hash,file);
          send_file(file, buffer, sock);
        }
        
        /*Carry out query request*/
        else if(strncmp(sendbuff,"query",5)==0){
          sscanf(sendbuff,"%s %s %s %s", garbage,hash,hash2,file);

          /*Compare dates*/
          struct stat *file_info = malloc(sizeof(struct stat));
          if (lstat(file, file_info) != 0) {
            perror("lstat() error");
            exit(EXIT_FAILURE);
          }
          char date[BUFFER_SIZE];
          time_t val = file_info->st_mtime;
          strftime(date, BUFFER_SIZE, "%Y %m %d %H %M %S", localtime(&val));

          char servdate[BUFFER_SIZE];
          n = recv(sock, servdate, BUFFER_SIZE, 0);
          if ( n < 0 ){
            perror( "recv() failed\n");
            exit(EXIT_FAILURE);
          }
          int cli[8];
          int ser[8];
          int j = 0;
          int p = 0;
          sscanf(date,"%d %d %d %d %d %d %d %d",&cli[0],&cli[1],&cli[2],&cli[3],&cli[4],&cli[5],&cli[6],&cli[7]);
          sscanf(servdate,"%d %d %d %d %d %d %d %d",&ser[0],&ser[1],&ser[2],&ser[3],&ser[4],&ser[5],&ser[6],&ser[7]);
          for(;j<8;j++){

            if(cli[j] > ser[j]){
              sprintf(sendbuff,"%s %s %s\n", "put", hash2, file);
              p = 1;
              break; 
            }
            else if(ser[j] > cli[j]){
              sprintf(sendbuff,"%s %s %s\n", "get", hash, file);
              break;
            }
          }
          if(j == 8){
            sprintf(sendbuff,"%s %s %s\n", "get", hash2, file);
          }

          n = send(sock, sendbuff, BUFFER_SIZE, 0); 
          if (n < 0){
            perror( "send() failed\n");
            exit(EXIT_FAILURE);
          }

          /*If client file is newer carry out put request*/
          if(p == 1){
            send_file(file, buffer, sock);
          }

          /*If server file is newer carry out put request*/
          else{
            recv_file(file, buffer,sock);
          }
        }
      }
    }
    close(sock);
}

int send_file(char* file, char* buffer, int newsock){
  int fd;
  struct stat file_stat;
  char file_size[BUFFER_SIZE];
  int remain_data;
  int seg_size;
  int sent_bytes = 0;
  int offset;

  fd = open(file, O_RDONLY);
  if (fd == -1){
    perror( "open() failed" );
    exit( EXIT_FAILURE );
  }

  /* Get file stats */
  if (fstat(fd, &file_stat) < 0){
      perror( "fstat() failed" );
    exit( EXIT_FAILURE );
  }

  sprintf(file_size, "%lld", file_stat.st_size);

  /* Sending file size */
  int len = send(newsock, file_size, BUFFER_SIZE, 0);
  fflush(NULL);
  if (len < 0){
    perror( "send() failed" );
    exit( EXIT_FAILURE );
  }

  offset = 0;
  remain_data = file_stat.st_size;

  /* Sending file data */
  int bytes_read = read(fd, buffer, BUFFER_SIZE);

  if (bytes_read < 0) {
      perror( "recv() failed" );
    exit( EXIT_FAILURE );
  }

  seg_size = BUFFER_SIZE;
  if(remain_data <= BUFFER_SIZE){
    seg_size = remain_data;
  }

  sent_bytes = send(newsock, buffer, BUFFER_SIZE, 0);

  while (remain_data > 0 && sent_bytes > 0){
    fflush(NULL);
    //printf(stdout, "1. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
    remain_data -= sent_bytes;
    //fprintf(stdout, "2. Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);

    if(remain_data <= BUFFER_SIZE){
      seg_size = remain_data;
    }

    int bytes_read = read(fd, buffer, BUFFER_SIZE);
    if (bytes_read == 0) // We're done reading from the file
        break;

    if (bytes_read < 0){
      perror("read() failed");
      exit( EXIT_FAILURE );
    }
    sent_bytes = send(newsock, buffer, BUFFER_SIZE, 0);
  }
  close(fd);
  return 0;
}

int recv_file(char* file, char* buffer, int sock){

  int file_size;
  FILE *received_file;
  int remain_data = 0;
  ssize_t len;

  /*Receive file size*/
  int n = recv(sock, buffer, BUFFER_SIZE,0);
  if ( n < 0 ){
    perror( "read failed\n");
  }
  file_size = atoi(buffer);
  //fprintf(stdout, "File size : %d\n", file_size);

  received_file = fopen(file, "w");
  if (received_file == NULL){
    perror( "fopen() failed" );
    exit( EXIT_FAILURE );
  }

  remain_data = file_size;
  int seg_size = BUFFER_SIZE;
  if(remain_data < BUFFER_SIZE){
    seg_size = remain_data;
  }

  while ((remain_data > 0) &&((len = recv(sock, buffer, BUFFER_SIZE,0)) > 0) && (remain_data > 0)){
      fwrite(buffer, sizeof(char), seg_size, received_file);
      remain_data -= len;
      if(remain_data <= BUFFER_SIZE){
        seg_size = remain_data;
      }
      //fprintf(stdout, "Receive %zd bytes:- %d bytes\n", len, remain_data);
  }

  fclose(received_file);
  return 0;
}






