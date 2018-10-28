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
#include "../utilities/stopwatch.h"
#include "server_connection.h"
#include "routing_table.h"


using namespace std;

typedef pair<int, struct sockaddr_in> Connection;
typedef pair<int, string> Pair;


class Server
{
	private:
	/* Variables */
	map<int, ServerConnection> neighbors;
	map<int, string> usernames;
	set<string> usernames_set;
	string id;
	fd_set active_set;
	char SOH = '\x01';
	char EOT = '\x04';
	int neighbor_connections;
	int MAX_BUFFER_SIZE;
	int MAX_NEIGHBOUR_CONNECTIONS;
	int max_file_descriptor;
	
	StopWatch keepalive_timer;
	StopWatch routing_table_timer;
	StopWatch clear_routes_timer;

	pair<int, struct sockaddr_in> server_conn_port;
	pair<int, struct sockaddr_in> client_conn_port;
	pair<int, struct sockaddr_in> udp_port;
	map<int, string> hashes;
	RoutingTable routing_table;

	/* Methods */

	/* getters/setters */
	string get_id();

	/* Connected client realted methods */
	void display_commands(int fd);
	void display_users(int fd);
	bool add_user(int fd, string username, string& feedback_message);
	void send_to_all(string message);
	void send_to_user(int rec_fd, string message);
	int get_fd_by_user(string username);
	bool user_exists(int fd);
	void remove_from_set(string username);
	void disconnect_user(int fd);
	void service_tcp_client_request(int fd);

	/* Connected server related methods */
	bool is_server_in_list(int port);
	bool response_is_id(string response);
	bool response_is_listservers(string response);
	bool is_neighbor(string server_id);
	bool is_server(int fd);
	string listservers();
	void add_to_serverlist(int fd, struct sockaddr_in& address, string server_id);
	void update_neighbor(int fd, string id, string ip, int port);
	void update_server_id(int fd, string new_id);
	void update_server_port(int fd, int port);
	void accept_incomming_server(int fd, struct sockaddr_in& address);
	void connect_to_server(string sub_command);
	void disconnect_server(int fd);
	void service_tcp_server_request(int fd);
	int get_server_fd_by_id(string id);
	void run_scheduled_tasks();
	void send_keepalive(int fd);
	void send_keepalive_to_all();
	void update_routes();

	/* Client/Server connection related methods */
	void write_to_fd(int fd, string message);
	void update_max_fd(int fd);
	int accept_connection(int fd, struct sockaddr_in& address);
	void select_wrapper(fd_set& set);
	void service_udp_request(int fd);
	void receive_from_client_or_server(int fd);
	void execute_command(int fd, string buffer, string from_server_id = "");
	
  public:
	Server(int server_p, int client_p, int udp_p, string server_id);
	int run();
	void set_max_buffer(int size);
};

#endif