#ifndef SOCKET_UTILITIES_H
#define SOCKET_UTILITIES_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <string.h>
#include <errno.h>

#include <vector>

typedef std::pair<int, struct sockaddr_in> Connection;

/**
 * Utilities for socket functionality
*/
namespace socket_utilities
{

	void error(const char *message);

	int create_tcp_socket();

	int create_udp_socket();

	int bind_to_address(int fd, struct sockaddr_in& address, int port);

	void listen_on_socket(int fd);
	
	void close_socket(int fd);

	int write_to_client(int fd, std::string message);

	int connect(int fd, sockaddr_in& address);

	int find_available_port(Connection& connection, int min_port, int max_port);
} 

#endif