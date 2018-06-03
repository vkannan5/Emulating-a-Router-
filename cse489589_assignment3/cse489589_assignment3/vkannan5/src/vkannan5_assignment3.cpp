#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <strings.h>
#include <time.h>
#include "../include/global.h"
#include "../include/control_header_lib.h"
#include "../include/network_util.h"
#include <stdlib.h>
#include <cstdlib>
#include <sys/socket.h>
#include <sys/queue.h>
#include <inttypes.h>
#include "../include/connection_manager.h"
#include "../include/pa3.h"
#include "../include/init.h"
#include <sstream>

using namespace std;

struct timeval tv;
bool crash=false;


bool control_recv_hook(int sock_index)
{   
    char *cntrl_header, *cntrl_payload;    
    uint8_t control_code;
    int payload_len;
    timespec ts;
    
/* Get control header */
    cntrl_header = (char *) malloc(sizeof(char)*CNTRL_HEADER_SIZE);
    bzero(cntrl_header, CNTRL_HEADER_SIZE);
    //printf("Calling recv all\n");
    if(recvALL(sock_index, cntrl_header, CNTRL_HEADER_SIZE) < 0){
        remove_control_conn(sock_index);
        free(cntrl_header);
        return FALSE;
    }

    /* Get control code and payload length from the header */
    #ifdef PACKET_USING_STRUCT
       BUILD_BUG_ON(sizeof(struct CONTROL_HEADER) != CNTRL_HEADER_SIZE); // This will FAIL during compilation itself; See comment above.

        struct CONTROL_HEADER *header = (struct CONTROL_HEADER *) cntrl_header;
        control_code = header->control_code;
        payload_len = ntohs(header->payload_len);
    #endif
    #ifndef PACKET_USING_STRUCT
        memcpy(&control_code, cntrl_header+CNTRL_CONTROL_CODE_OFFSET, sizeof(control_code));
        memcpy(&payload_len, cntrl_header+CNTRL_PAYLOAD_LEN_OFFSET, sizeof(payload_len));
         payload_len = ntohs(payload_len);
    #endif
    
    free(cntrl_header);
    
    /* Get control payload */
    if(payload_len != 0){ 
        cntrl_payload = (char *) malloc(sizeof(char)*payload_len);
        bzero(cntrl_payload, payload_len);
        
        if(recvALL(sock_index, cntrl_payload, payload_len) < 0){
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return FALSE;
        }
         
    }
    int i, len=0;
    //char send_buffer[1024]="Hi from the other side";
    time_t mytime;
        
    switch(control_code){
        case 0: if(!crash)
                {
                //printf("case 0 called\n");
                author_response(sock_index);                
                }
                break;

        case 1: if(!crash)
                {//printf("Case 1 called\n");
                clock_t start=clock();
                //double time_ns=(double)(start)*1000.0/CLOCKS_PER_SEC;
                //printf("Time at Init is %f",time_ns);
                init_response(sock_index);
                get_Information(cntrl_payload);
                create_router_sock();
                create_data_sock();
                FD_SET(router_socket, &master_list);
                head_fd=router_socket;
                FD_SET(data_socket, &master_list);
                head_fd = data_socket;    
                tv.tv_sec=Periodic_Interval;
                }
                break;

        case 2: if(!crash)
                {//printf("case 2 called\n");
                init_control_response(sock_index);
                }
                break; 
        case 3: if(!crash)
                {//printf("Case 3 called\n");
                get_link_update(cntrl_payload);
                init_response(sock_index);
                }
                break;
        case 4: if(!crash) 
                {//printf("case 4 called\n");
                crash=true;
                init_response(sock_index); 
                }   
        case 5: if(!crash)
                {
                    //printf("case 5 called\n");
                    get_file_information(cntrl_payload, payload_len,sock_index);
                }
        case 7: if(!crash)
                {
                    //printf("case 5 called\n");
                    last_information(sock_index);
                }
        case 8: if(!crash)
                {
                    //printf("case 5 called\n");
                    penultimate_information(sock_index);
                }
    }

    if(payload_len != 0) free(cntrl_payload);
    return TRUE;
}

void main_loop()
{
    int selret, sock_index, fdaccept, bytes_recvd=0;
    struct sockaddr_in client_Address;
    char client_ip[2048];//=(char *)malloc(sizeof(char)*1024);
    char recv_buffer[2048];//=(char *)malloc(sizeof(char)*1024);
    char recv_buffer2[2048];

    while(TRUE){
        watch_list = master_list;
        selret = select(head_fd+1, &watch_list, NULL, NULL, &tv);

        if(selret < 0)
            ERROR("select failed.");

        if(selret==0)
        {
          /*------------------SEND DATA TO NEIGHBORS-------------------------*/    
            if(!crash)
            {
              //printf("Select timed out. \n");
              UDP_Send();          
              tv.tv_sec=Periodic_Interval; 

              //tv.tv_sec=10000;
            }  
          
        }

        /* Loop through file descriptors to check which ones are ready */
        for(sock_index=0; sock_index<=head_fd; sock_index+=1){

            if(!crash)
            {

            if(FD_ISSET(sock_index, &watch_list)){

                /* control_socket */
                if(sock_index == control_socket){
                    fdaccept = new_control_conn(sock_index);

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if(fdaccept > head_fd) head_fd = fdaccept;
                }

                /* router_socket */

                else if(sock_index == router_socket){
                    /*call handler that will call recvfrom() .....*/
                      //printf("Receiving Information\n");
                      memset(recv_buffer, 0,2048);
                      memset(client_ip, 0,2048);
                      addrlen=sizeof(struct sockaddr);
                      int count2=0;
                      int byt= (6*Number_of_routers) + 4;
                      //printf("byt is %d\n", byt);
                      bytes_recvd = recvfrom(router_socket,recv_buffer,2048,0,(struct sockaddr *)&client_Address, &addrlen);
                      
                      if(bytes_recvd<=0)
                      {
                        //printf("Error in receiving\n");
                      }
                      getRoutingUpdateInfo(recv_buffer);
                  }

                /* data_socket */
                else if(sock_index == data_socket){
                    //fdaccept = new_control_conn(sock_index);
                    //printf("*******Receiving TCP data*********\n");
                    fdaccept2 = new_TCP_conn(sock_index);
                    //TCP_DATA_FWD(fdaccept2);
                    memset(&TCP_payload,0,sizeof(TCP_payload));
                    int bytes = recv(fdaccept2, TCP_payload, 1036,0);
                    //printf("The number of bytes rcvd is :: %d\n",bytes);
                    //printf("Received %s\n", &TCP_payload[12]);
                    received_TCP();
                
                    /* Add to watched socket list */
                    FD_SET(fdaccept2, &master_list);
                    if(fdaccept2 > head_fd) head_fd = fdaccept2;

                }

                /* Existing connection */
                else{
                    if(sock_index==fdaccept2)
                    {
                    memset(&TCP_payload,0,sizeof(TCP_payload));
                    int bytes = recv(fdaccept2, TCP_payload, 1036,0);
                    //printf("The number of bytes rcvd is :: %d\n",bytes);
                    //printf("Received %s\n", &TCP_payload[12]);
                    received_TCP();
                    }
                    else if(sock_index){
                        if(!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);

                    }
                    /*else if isData(sock_index);*/
                    else ERROR("Unknown socket index");
                } 
            } 
            } 
        }
    }
}
 
void init()
{
    control_socket = create_control_sock();

    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd=control_socket;
    tv.tv_sec=10000;
    main_loop();
}

int main(int argc, char **argv)
{
    /*Start Here*/

    CONTROL_PORT=atoi(argv[1]);
    //printf("Control port is %u\n", CONTROL_PORT);
    init(); // Initialize connection manager; This will block

    return 0;
}
