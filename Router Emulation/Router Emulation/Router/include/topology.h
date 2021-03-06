#ifndef TOPOLOGY_H_
#define TOPOLOGY_H_
#include <vector>

uint16_t no_of_routers;
uint16_t periodic_interval;
char* ip_address[5];
uint16_t dist_vect[5][5];

struct router_topology{
	uint16_t router_id;
	uint16_t router_port;
	uint16_t data_port;
	uint16_t cost;
	uint32_t router_ip_address;
	int neighbour;
	char *ip_address;
};	

struct routing_table{
	uint16_t router_id;
	uint16_t padding;
	uint16_t next_hop_id;
	uint16_t cost;	
};
struct time_track{
	uint16_t router_id;
	clock_t init_time;
	clock_t start_time;
	clock_t after_sel_time;
};
struct update_routing_table{
	uint32_t router_ip_address;
	uint16_t router_port;
	uint16_t padding;
	uint16_t router_id;
	uint16_t cost;	
};
struct send_to_router{
	uint16_t no_of_update_fields;
	uint16_t source_router_port;
	uint32_t source_router_ip_address;
	update_routing_table table[5];
};
update_routing_table updated_routing_table[10];
send_to_router send_routing_table;	
send_to_router recv_routing_table;		
#endif
