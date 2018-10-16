#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>
#include <set>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#include "../utilities/string_utilities.h"
#include "../utilities/socket_utilities.h"
#include "../utilities/time_utilities.h"
#include "buffer_content.h"

typedef std::pair<int, struct sockaddr_in> Connection;
typedef std::pair<int, std::string> Client;


class Server
{
	private:
	/* Variables */
	std::map<int, std::string> usernames;
	std::set<std::string> usernames_set;
	std::string id;

	std::pair<int, struct sockaddr_in> server_conn_port;
	std::pair<int, struct sockaddr_in> client_conn_port;
	std::pair<int, struct sockaddr_in> udp_port;
	struct sockaddr_in client;
	socklen_t client_length;

	fd_set active_set, read_set; // might want to add write_set
	
	int MAX_BUFFER_SIZE;
	int max_file_descriptor;
	
	/* Methods */
	/* getters/setters */
	std::string get_id();
	/* API COMMAND methods */
	void display_commands(int fd);
	void display_users(BufferContent& buffer_content);
	bool add_user(BufferContent& buffer_content, std::string& feedback_message);
	void send_to_all(BufferContent& buffer_content);
	void send_to_user(int rec_fd, BufferContent& buffer_content);
	
	void parse_buffer(char * buffer, int fd);
	void execute_command(BufferContent& buffer_content);
	
	/* helper methods */
	int get_fd_by_user(std::string username);
	bool user_exists(int fd);
	void write_to_client(int fd, std::string message);
	void remove_from_set(std::string username);
	void update_max_fd(int fd);
	int accept_connection(int socket, sockaddr_in& address, socklen_t & length);

  public:
	Server();
	int run();
	void set_max_buffer(int size);
};

#endif