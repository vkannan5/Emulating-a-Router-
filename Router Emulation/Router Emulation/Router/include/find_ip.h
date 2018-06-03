#include <string.h>
#include <stdio.h>
#include <iostream>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <inttypes.h>

#define EPHEMERAL_PORT 53
char* str = (char*)malloc(100);
char* find_ip()
{
    struct sockaddr_in udp;
    int temp_udp =socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int len = sizeof(udp);
    //char str[INET_ADDRSTRLEN];
   // printf("Temp udp %d\n",temp_udp);
    if (temp_udp == -1)
    {
        //printf("Socket creation failed!");
    }
    
    memset((char *) &udp, 0, sizeof(udp));
    udp.sin_family = AF_INET;
    udp.sin_port = htons(EPHEMERAL_PORT);
    inet_pton(AF_INET, "8.8.8.8", &udp.sin_addr);
    //udp.sin_addr.s_addr = inet_addr("8.8.8.8");
    
    if (connect(temp_udp, (struct sockaddr *)&udp, sizeof(udp)) < 0)
    {
        //printf("\nConnection Failed \n");
    }
    if (getsockname(temp_udp,(struct sockaddr *)&udp,(unsigned int*) &len) == -1)
    {
        perror("getsockname");
    }
    
    inet_ntop(AF_INET, &(udp.sin_addr), str, len);
    //printf("%s", str);
    
    
    return str;
}

