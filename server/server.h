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

typedef std::vector<std::pair<int, struct sockaddr_in> > Servers;
typedef std::pair<int, struct sockaddr_in> Pair;


class Server
{
	private:
	Servers servers;
	std::map<int, std::string> usernames;
	std::set<std::string> usernames_set;
	// std::map<int, int> port_knocks;
	time_t knock_start, knock_stop;
	int MAX_BUFFER_SIZE;
	fd_set active_set, read_set; // might want to add write_set
	int max_file_descriptor;
	std::string id;

	struct hostent* destination_server;


	void display_commands(int fd);
	bool add_user(BufferContent& buffer_content, std::string& feedback_message);
	void send_to_all(BufferContent& buffer_content);
	void display_users(BufferContent& buffer_content);
	bool user_exists(int fd);
	int get_fd_by_user(std::string username);
	void send_to_user(int rec_fd, BufferContent& buffer_content);
	void write_to_client(int fd, std::string message);
	void remove_from_set(std::string username);
	void set_fortune();
	std::string get_fortune();

  public:
	Server();
	int run();
	void set_timer();
	void stop_timer();
	int get_time_in_seconds();
	void set_max_buffer(int size);
	void create_sockets();
	void add_to_active_set();
	void listen_to_sockets();
	void rebind(Pair& pair);
	void update_max_fd(int fd);
	int accept_connection(int socket, sockaddr_in& address, socklen_t & length);
	void parse_buffer(char * buffer, int fd);
	void execute_command(BufferContent& buffer_content);
	void set_scan_destination(std::string dest_host);
	void knock_port(int port, char* buffer);
	void scan(int min_port, int max_port, bool scan_when_running);
};

#endif