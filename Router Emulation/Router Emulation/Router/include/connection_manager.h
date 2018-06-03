//#include "../network_util.h"

#ifndef CONNECTION_MANAGER_H_
#define CONNECTION_MANAGER_H_
/*
#include <sys/select.h>

#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
*/
int control_socket, router_socket, data_socket;

void init();

#endif
