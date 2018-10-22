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
#include "server_connection.h"

using namespace std;

typedef pair<int, struct sockaddr_in> Connection;
typedef pair<int, string> Pair;


class Server
{
	private:
	/* Variables */
	map<int, ServerConnection> neighbors;

	// map<string, int> neighbors_by_conninfo;
	

	int neighbor_connections;
	map<int, string> usernames;
	set<string> usernames_set;
	string id;

	map<int, string> hashes;
	pair<int, struct sockaddr_in> server_conn_port;
	pair<int, struct sockaddr_in> client_conn_port;
	pair<int, struct sockaddr_in> udp_port;

	fd_set active_set;
	
	int MAX_BUFFER_SIZE;
	int MAX_NEIGHBOUR_CONNECTIONS;
	int max_file_descriptor;

	char SOH = '\x01';
	char EOT = '\x04';
	
	/* Methods */
	/* getters/setters */
	string get_id();
	/* API COMMAND methods */
	void display_commands(int fd);
	void display_users(BufferContent& buffer_content);
	bool add_user(BufferContent& buffer_content, string& feedback_message);
	void send_to_all(BufferContent& buffer_content);
	void send_to_user(int rec_fd, BufferContent& buffer_content);
	
	void parse_buffer(string buffer, int fd, string from_server_id = "");
	void execute_command(BufferContent& buffer_content, string from_server_id = "");
	
	bool response_is_id(string response);
	bool response_is_listservers(string response);
	/* helper methods */
	int get_fd_by_user(string username);
	bool user_exists(int fd);
	bool is_neighbor(string server_id);
	bool is_server(int fd);
	void write_to_fd(int fd, string message);
	void remove_from_set(string username);
	void update_max_fd(int fd);
	int accept_connection(int fd, struct sockaddr_in& address);
	string listservers();
	void accept_incomming_server(int fd, struct sockaddr_in& address);
	void connect_to_server(BufferContent& buffer_content);
	void add_to_serverlist(int fd, struct sockaddr_in& address, string server_id);
	void disconnect_user(BufferContent& buffer_content);
	void select_wrapper(fd_set& set);
	void service_udp_request(int fd);
	void service_tcp_server_request(int fd);
	void service_tcp_client_request(int fd);
	void receive_from_client_or_server(int fd);
	void fetch_id_from_fd(int fd);
	void update_server_id(int fd, string new_id);
  public:
	Server(int server_p, int client_p, int udp_p, string server_id);
	int run();
	void set_max_buffer(int size);
};

#endif