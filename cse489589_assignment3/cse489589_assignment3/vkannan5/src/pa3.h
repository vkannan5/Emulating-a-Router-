#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <strings.h>
//#include "../include/global.h"
//#include "../include/control_header_lib.h"
//#include "../include/network_util.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <inttypes.h>
#include <vector>   
//#include "../include/connection_manager.h"
using namespace std;

#ifndef PACKET_USING_STRUCT
    #define CNTRL_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_PAYLOAD_LEN_OFFSET 0x06
#endif

#ifndef PACKET_USING_STRUCT
    #define CNTRL_RESP_CONTROL_CODE_OFFSET 0x04
    #define CNTRL_RESP_RESPONSE_CODE_OFFSET 0x05
    #define CNTRL_RESP_PAYLOAD_LEN_OFFSET 0x06
#endif

#define TRUE 1
#define FALSE 0
#define AUTHOR_STATEMENT "I, vkannan5, have read and understood the course academic integrity policy."
#define INIT_STATEMENT ""
#define INFINITY 65535;

/*----------------------------------------------------------------------------------------------------------------------------------------------------------*/

uint16_t Number_of_routers, Periodic_Interval, Router_ID[10], Router_Port1[10], Router_Port2[10], Cost[10], RoutingTable[6][1], Forwarding_Table[6][3];
uint32_t Router_IP[10];
string init_control_response_payload;
char *Cntrl_resp_data;
char buffer[10][INET_ADDRSTRLEN + 1], buffer2[INET_ADDRSTRLEN + 1], IP[10][1024];
int selfID, retval;
struct timeval tv, timers[5];
vector<int> neighbors_id;
/*----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/* Linked List for active control connections */
struct ControlConn
{
    int sockfd;
    LIST_ENTRY(ControlConn) next;
}*connection, *conn_temp;
LIST_HEAD(ControlConnsHead, ControlConn) control_conn_list;

fd_set master_list, watch_list;
int head_fd;

unsigned conc_integers(unsigned int a, unsigned int b) {
    unsigned int power = 10;
    while(b >= power)
        power *= 10;
    return a * power + b;        
}

/**Reference: http://www.geeksforgeeks.org/reverse-words-in-a-given-string/**/  

void reverse(char *begin, char *end)
{
  char temp;
  while (begin < end)
  {
    temp = *begin;
    *begin++ = *end;
    *end-- = temp;
  }
}

void reverseWords(char *s)
{
  char *word_begin = s;
  char *temp = s;

  while( *temp )
  {
    temp++;
    if (*temp == '\0')
    {
      reverse(word_begin, temp-1);
    }
    else if(*temp == '.')
    {
      reverse(word_begin, temp-1);
      word_begin = temp+1;
    }
  } 
 
  reverse(s, temp-1);
}

char* create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len)
{
    char *buffer;
    #ifdef PACKET_USING_STRUCT
        /** ASSERT(sizeof(struct CONTROL_RESPONSE_HEADER) == 8) 
          * This is not really necessary with the __packed__ directive supplied during declaration (see control_header_lib.h).
          * If this fails, comment #define PACKET_USING_STRUCT in control_header_lib.h
          */
        BUILD_BUG_ON(sizeof(struct CONTROL_RESPONSE_HEADER) != CNTRL_RESP_HEADER_SIZE); /* This will FAIL during compilation itself; See comment above.*/

        struct CONTROL_RESPONSE_HEADER *cntrl_resp_header;
    #endif
    #ifndef PACKET_USING_STRUCT
        char *cntrl_resp_header;
    #endif

    struct sockaddr_in addr;
    socklen_t addr_size;

    buffer = (char *) malloc(sizeof(char)*CNTRL_RESP_HEADER_SIZE);
    #ifdef PACKET_USING_STRUCT
        cntrl_resp_header = (struct CONTROL_RESPONSE_HEADER *) buffer;
    #endif
    #ifndef PACKET_USING_STRUCT
        cntrl_resp_header = buffer;
    #endif

    addr_size = sizeof(struct sockaddr_in);
    getpeername(sock_index, (struct sockaddr *)&addr, &addr_size);

    #ifdef PACKET_USING_STRUCT
        /* Controller IP Address */
        memcpy(&(cntrl_resp_header->controller_ip_addr), &(addr.sin_addr), sizeof(struct in_addr));
        /* Control Code */
        cntrl_resp_header->control_code = control_code;
        /* Response Code */
        cntrl_resp_header->response_code = response_code;
        /* Payload Length */
        cntrl_resp_header->payload_len = htons(payload_len);
    #endif

    #ifndef PACKET_USING_STRUCT
        /* Controller IP Address */
        memcpy(cntrl_resp_header, &(addr.sin_addr), sizeof(struct in_addr));
        /* Control Code */
        memcpy(cntrl_resp_header+CNTRL_RESP_CONTROL_CODE_OFFSET, &control_code, sizeof(control_code));
        /* Response Code */
        memcpy(cntrl_resp_header+CNTRL_RESP_RESPONSE_CODE_OFFSET, &response_code, sizeof(response_code));
        /* Payload Length */
        payload_len = htons(payload_len);
        memcpy(cntrl_resp_header+CNTRL_RESP_PAYLOAD_LEN_OFFSET, &payload_len, sizeof(payload_len));
    #endif

    return buffer;
}

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes)
{		int i;
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);
		//printf("HERE 4\n");
    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += recv(sock_index, buffer+bytes, nbytes-bytes, 0);
    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes)
{
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);

    if(bytes == 0) return -1;
    while(bytes != nbytes)
        bytes += send(sock_index, buffer+bytes, nbytes-bytes, 0);

    return bytes;
}

void author_response(int sock_index)
{	//printf("HERE 3\n");
    printf("author_response called\n");
	uint16_t payload_len, response_len;
	char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

	payload_len = sizeof(AUTHOR_STATEMENT)-1; // Discount the NULL chararcter
	cntrl_response_payload = (char *) malloc(payload_len);
	memcpy(cntrl_response_payload, AUTHOR_STATEMENT, payload_len);

	cntrl_response_header = create_response_header(sock_index, 0, 0, payload_len);

	response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
	cntrl_response = (char *) malloc(response_len);
	/* Copy Header */
	memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
	free(cntrl_response_header);
	/* Copy Payload */
	memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
	free(cntrl_response_payload);

	sendALL(sock_index, cntrl_response, response_len);

	free(cntrl_response);
}

int create_control_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("bind() failed");

    if(listen(sock, 5) < 0)
        ERROR("listen() failed");

    LIST_INIT(&control_conn_list);

    return sock;
}

int create_router_sock()
{
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
        ERROR("socket() failed");

    /* Make socket re-usable */
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0)
        ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
    control_addr.sin_port = htons(ROUTER_PORT); 

    if(bind(sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0)
        ERROR("bind() failed");

    return sock;
}

int new_control_conn(int sock_index)
{
    int fdaccept;
    socklen_t caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *)&remote_controller_addr, &caddr_len);
    if(fdaccept < 0)
        ERROR("accept() failed");

    /* Insert into list of active control connections */
    connection = (struct ControlConn*) malloc(sizeof(struct ControlConn));
    connection->sockfd = fdaccept;
    LIST_INSERT_HEAD(&control_conn_list, connection, next);

    return fdaccept;
}

void remove_control_conn(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next) {
        if(connection->sockfd == sock_index) LIST_REMOVE(connection, next); // this may be unsafe?
        free(connection);
    }

    close(sock_index);
}

bool isControl(int sock_index)
{
    LIST_FOREACH(connection, &control_conn_list, next)
        if(connection->sockfd == sock_index) return TRUE;

    return FALSE;
}

void init_response(int sock_index)
{printf("Init response called\n");
  uint16_t payload_len, response_len;
  char *cntrl_response_header, *cntrl_response_payload, *cntrl_response;

  payload_len = sizeof(INIT_STATEMENT)-1; // Discount the NULL chararcter
  cntrl_response_payload = (char *) malloc(payload_len);
  memcpy(cntrl_response_payload, INIT_STATEMENT, payload_len);

  cntrl_response_header = create_response_header(sock_index, 0, 0, payload_len);

  response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
  cntrl_response = (char *) malloc(response_len);
  /* Copy Header */
  memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
  free(cntrl_response_header);
  /* Copy Payload */
  memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);
  free(cntrl_response_payload);

  sendALL(sock_index, cntrl_response, response_len);

  free(cntrl_response);
}


void init_control_response(int sock_index)
{   printf("init_control_response called \n");
  uint16_t payload_len, response_len;
  char *cntrl_response_header;
  char *cntrl_response_payload;//[40];
  char *cntrl_response;
  int i,j;
  //int copied=0;
  cntrl_response_payload=(char *)malloc(sizeof(uint16_t)*20);
  
  //printf("size of unit16 %u \n", sizeof(uint16_t));
  int count=0;
  
  for(i=1;i<=Number_of_routers;i++)
  {
    for(j=0;j<3;j++)
    {
      uint16_t temp=htons(Forwarding_Table[i][j]);
      memcpy(&cntrl_response_payload[2*count], &temp, 2);
      count++;
      if(j==0)
      { //padding
      uint16_t temp2=0;
      uint16_t temp3=htons(temp2);
      memcpy(&cntrl_response_payload[2*count], &temp3, 2);
      count++;
      }      
    }
  }
  payload_len=8*Number_of_routers;//.size();//-1;
  cntrl_response_header = create_response_header(sock_index, 0, 0, payload_len);
  response_len = CNTRL_RESP_HEADER_SIZE+payload_len;
  cntrl_response = (char *) malloc(response_len);  
  memcpy(cntrl_response, cntrl_response_header, CNTRL_RESP_HEADER_SIZE);
  free(cntrl_response_header);  
  memcpy(cntrl_response+CNTRL_RESP_HEADER_SIZE, cntrl_response_payload, payload_len);  
  sendALL(sock_index, cntrl_response, response_len);
  free(cntrl_response);
}

/**Reference: https://stackoverflow.com/questions/4716389/c-convert-uint32-ip-address-to-text-x-x-x-x**/
char* GetIPString(uint32_t x, int i) 
{   
    //buffer2="";
    //printf("GetIPString called \n");
    memset(buffer2, 0,sizeof(buffer2));
    inet_ntop(AF_INET, &x, buffer2, sizeof(buffer2));
    reverseWords(buffer2);
    //cout<< "IP Buffer:"<<buffer2<<endl;
    strncpy(IP[i], buffer2, sizeof(buffer2));
    return buffer2;
}


void printIP()
{
  int i;
  for(i=1;i<6;i++)
  {
    cout<<"IP:"<<i<<IP[i]<<endl;
  }
}

void getInformation(char *cntrl_payload)
{
    int i;
    printf("getInformation called\n");
    Number_of_routers=(uint16_t)((uint8_t)cntrl_payload[1] | (uint8_t)cntrl_payload[0]<<8);
    Periodic_Interval=(uint16_t)((uint8_t)cntrl_payload[3] | (uint8_t)cntrl_payload[2]<<8);
    //cout<<"Number_of_routers:"<<Number_of_routers<<" Periodic_Interval:"<<Periodic_Interval<<endl;
    for(i=0;i<Number_of_routers;i++)
    {               
          Router_ID[i+1]=(uint16_t)((uint8_t)cntrl_payload[5+i*12] | (uint8_t)cntrl_payload[4+i*12]<<8);
          cout<<endl;
          //cout<<"Router ID "<<Router_ID[i]<<endl;
          Router_Port1[i+1]=(uint16_t)((uint8_t)cntrl_payload[7+i*12] | (uint8_t)cntrl_payload[6+i*12]<<8);
          //cout<<"Router Port1 "<<Router_Port1[i]<<endl;
          Router_Port2[i+1]=(uint16_t)((uint8_t)cntrl_payload[9+i*12] | (uint8_t)cntrl_payload[8+i*12]<<8);
          //cout<<"Router Port2 "<<Router_Port2[i]<<endl;
          Cost[i+1]=(uint16_t)((uint8_t)cntrl_payload[11+i*12] | (uint8_t)cntrl_payload[10+i*12]<<8);
          //cout<<"Cost "<<Cost[i]<<endl;
          Router_IP[i+1] = (uint32_t)((uint8_t)cntrl_payload[12+i*12] << 24 |(uint8_t)cntrl_payload[13+i*12] << 16 |  (uint8_t)cntrl_payload[14+i*12] << 8  |   (uint8_t)cntrl_payload[15+i*12]);
          GetIPString(Router_IP[i+1],i+1);  
          printIP();                    
    } 
}


void PrintRoutingTable()
{
  int i;
  for(i=1;i<6;i++)
  {
    cout<<"selfID -> i :"<<RoutingTable[0][i]<<endl;
  }
}

void PrintForwardingTable()
{   printf("PrintForwardingTable called \n");
  int i,j;
  printf("Router ID \t Next Hop \t Cost \t\n");
  
  for(i=1;i<=Number_of_routers;i++)
  {
    for(j=0;j<3;j++)
    {
      printf("Forwarding_Table: %d ",Forwarding_Table[i][j]);
    } printf("\n");
  }
  //printf("payload length %u \n",init_control_response_payload.size());
  //cout<<endl<<"payload"<<init_control_response_payload<<endl;
}

void get_Routing_Table()
{
  int i;
  printf("get_Routing_Table called. Number_of_routers is %d\n", Number_of_routers);
  for(i=1;i<=Number_of_routers;i++)
  {
    if(Cost[i]==0)
    {
      selfID=i;
      printf("Self ID is %d\n",selfID);
      RoutingTable[i][0]=Router_ID[i];
      ROUTER_PORT=Router_Port1[i];
      Forwarding_Table[i][0]=Router_ID[i];
      Forwarding_Table[i][1]=Router_ID[i];
      Forwarding_Table[i][2]=Cost[i];
    }
    else if(Cost[i]==65535 || Cost[i]<0)
    {
      RoutingTable[i][0]=INFINITY;
      Forwarding_Table[i][0]=Router_ID[i];
      Forwarding_Table[i][1]=Cost[i];
      Forwarding_Table[i][2]=Cost[i];
    }
    else
    {
      RoutingTable[i][0]=Cost[i];
      neighbors_id.push_back(i);
      Forwarding_Table[i][0]=Router_ID[i];
      Forwarding_Table[i][1]=Router_ID[i];
      Forwarding_Table[i][2]=Cost[i];
      //neighbors_id.push_back(i);
    }
  }

  PrintForwardingTable();
}

void udp_sendto(char* cntrl_payload, char send_buffer[])
{   printf("UDP send called \n");
   int i;
   uint16_t id_ofrouter=(uint16_t)((uint8_t)cntrl_payload[1] | (uint8_t)cntrl_payload[0]<<8);
   cout<<"Router ID"<<id_ofrouter<<endl;
    uint16_t newcost = Periodic_Interval=(uint16_t)((uint8_t)cntrl_payload[3] | (uint8_t)cntrl_payload[2]<<8);
    cout<<"New cost:"<<newcost<<endl;
    /*
   if(!neighbors_id.empty())
   {
       int i;
       for(i=0;i<neighbors_id.size();i++)
       {
        printf("Neighbor: %d\n",neighbors_id[i]);
       }
/*
       for(i=0;i<neighbors_id.size();i++)
       {
           printf("sending data to neighbor %d",neighbors_id[i]);
           struct sockaddr_in sendto_Addr;
           socklen_t addr_size;
           sendto_Addr.sin_family = AF_INET;
           sendto_Addr.sin_port = htons(Router_Port1[neighbors_id[i]]);
           sendto_Addr.sin_addr.s_addr = inet_addr(IP[neighbors_id[i]]);  
           addr_size = sizeof(sendto_Addr);
           memset(sendto_Addr.sin_zero, '\0', sizeof(sendto_Addr.sin_zero)); 
           sendto(router_socket, send_buffer, sizeof(send_buffer), 0,(struct sockaddr *)&sendto_Addr,addr_size);
        }   
    }       */
   
}

