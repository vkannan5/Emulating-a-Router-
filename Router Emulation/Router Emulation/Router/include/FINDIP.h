#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
//#include "../include/logger.h"

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 1024
#define BUFSIZE 1024
//char* remoteIP;
char* printname(char IP[])
{

  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  inet_pton(AF_INET, IP, &sa.sin_addr);
  
  char *node=(char *)malloc(sizeof(char)*BUFSIZE);
  int res = getnameinfo((struct sockaddr*)&sa, sizeof(sa), node, sizeof(node), NULL, 0, 0);


  if (res)
  {

    exit(1);
  }

return node;
}

int printip()
{
    int sockfd;
    struct sockaddr_in remoteaddr;
    int client_socket;
    socklen_t addrlen= (socklen_t)sizeof(remoteaddr);
      char* remoteIP= (char *)malloc(sizeof(char) * BUFSIZE);//[INET6_ADDRSTRLEN];
//	char remoteIP[1024];
        char *ip_addr=(char *)malloc(sizeof(char) * 1024);
        int rv;

    client_socket=socket(AF_INET,SOCK_DGRAM,0);

    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_addr.s_addr = htonl(53);
    inet_pton(AF_INET, "8.8.8.8", &remoteaddr.sin_addr);

    if(connect(client_socket,(struct sockaddr *)&remoteaddr,sizeof(remoteaddr))==-1)
        {perror("Connect Error");
                exit(1);
        }

    getsockname(client_socket, (struct sockaddr*)&remoteaddr, &addrlen);

    inet_ntop(AF_INET, &remoteaddr.sin_addr, remoteIP, sizeof(remoteIP));

    printf("IP:%s\n", remoteIP);

    close(client_socket);

    //strcpy(ip_addr, remoteIP);
    return 0;
}

