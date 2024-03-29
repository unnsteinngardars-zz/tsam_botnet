#include "server.h"

/**
 * Initialize a Server with a tcp and udp port
 * TODO: when handing in, remove client_p and server_id
*/
Server::Server(int server_p, int client_p, int udp_p, string server_id)
{
	hashes.insert(Pair(1, "398e3848552c55d931d7ae4b48ffea40"));
	hashes.insert(Pair(2, "a181a603769c1f98ad927e7367c7aa51"));
	hashes.insert(Pair(3, "d80ea254f1b207e19040e7932e958d1c"));
	hashes.insert(Pair(4, "6864f389d9876436bc8778ff071d1b6c"));
	hashes.insert(Pair(5, "ca23ba209cc33678530392b7197fda4d"));

	id = server_id;
	FD_ZERO(&active_set);
	struct sockaddr_in s;
	struct sockaddr_in c;
	struct sockaddr_in u;

	routing_table.set_id(server_id);

	server_conn_port.second = s;
	client_conn_port.second = c;
	udp_port.second = u;
	
	server_conn_port.first = socket_utilities::create_tcp_socket(true);
	client_conn_port.first = socket_utilities::create_tcp_socket(true);
	udp_port.first = socket_utilities::create_udp_socket();

	socket_utilities::bind_to_address(server_conn_port, server_p);
	socket_utilities::bind_to_address(client_conn_port, client_p);
	socket_utilities::bind_to_address(udp_port, udp_p);

	socket_utilities::listen_on_socket(server_conn_port.first);
	socket_utilities::listen_on_socket(client_conn_port.first);

	FD_SET(server_conn_port.first, &active_set);
	FD_SET(client_conn_port.first, &active_set);
	FD_SET(udp_port.first, &active_set);
	max_file_descriptor = server_conn_port.first > client_conn_port.first ? server_conn_port.first : client_conn_port.first;
	max_file_descriptor = udp_port.first > max_file_descriptor ? udp_port.first : max_file_descriptor;
	MAX_NEIGHBOUR_CONNECTIONS = 5;
	neighbor_connections = 0;
}


/**
 * Get the ID of the server
*/
string Server::get_id()
{
	return id;
}

/**
 * CLIENT CONNECTION REALTED METHODS
 * Below methods relate to client connections
*/

/**
 * Display all commands
*/
void Server::display_commands(int fd)
{
	string help_message = "\nAvailable commands are:\n\n";
	help_message += "ID\t\t\tGet the ID of the server\n";
	//help_message += "CHANGE ID\t\tChange the ID of the server\n";
	help_message += "CONNECT,<username>\tIdentify yourself with username, cannot include space\n";
	help_message += "CS,<host>:<port>\tConnect to another server\n";
	help_message += "LISTSERVERS\t\tLists connected servers\n";
	help_message += "LEAVE\t\t\tLeave chatroom\n";
	help_message += "WHO\t\t\tGet list of connected users\n";
	help_message += "MSG,<user>,<message>\tSend a message to specific user\n";
	help_message += "MSG,ALL,<message>\tSend message to all users connected\n";
	help_message += "HELP\t\t\tSe available commands\n\n";
	write_to_fd(fd, help_message);
}

/**
 * Display all users
*/
void Server::display_users(int fd)
{
	string users = "\nLIST OF USERS:\n";
	set<string>::iterator it;
	for (it = usernames_set.begin(); it != usernames_set.end(); ++it){
		string username = *it;
		users += " " + username + "\n";
	}
	write_to_fd(fd, users);
}

/**
 * Add a newly connected user
*/
bool Server::add_user(int fd, string username, string& feedback_message)
{
	if (username.size() > 15 || !username.compare(""))
	{	
		feedback_message = "Username cannot be more than 15 characters\n";
		return false;
	}
	else if (usernames.count(fd) > 0)
	{
		feedback_message = "You have already connected with a username\n";
		return false;
	}
	else if (usernames_set.count(username) > 0)
	{
		feedback_message = "Username already taken\n";
		return false;
	}
	usernames.insert(Pair(fd, username));
	usernames_set.insert(username);
	feedback_message = username + " has logged in to the server\n";
	return true;
}

/**
 * Broadcast to all clients including yourself
*/
void Server::send_to_all(string message)
{	
	for (int i = 0; i <= max_file_descriptor; i++)
	{
		/* Check if the client is in the active set */
		if (FD_ISSET(i, &active_set))
		{
			if (i != server_conn_port.first && i != client_conn_port.first && i != udp_port.first)
			{
				write_to_fd(i, message);
			}
		}
	}
}

/**
 * Send a message to a single client
*/
void Server::send_to_user(int rec_fd, string message)
{
	if (FD_ISSET(rec_fd, &active_set)){
		write_to_fd(rec_fd, message);
	}
}

/**
 * Get a file descriptor by username
*/
int Server::get_fd_by_user(string username)
{
	int fd = -1;
	map<int, string>::iterator it;
	for (it = usernames.begin(); it != usernames.end(); ++it)
	{
		string user = it->second;
		if (!user.compare(username))
		{		
			fd = it->first;
		}
	}
	return fd;
}


/**
 * Check if a user exists
*/
bool Server::user_exists(int fd)
{
	if (usernames.count(fd) == 1) 
	{
		return true;
	}
	return false;
}

/**
 * Remove a user from the set of usernames
*/
void Server::remove_from_set(string username)
{
	set<string>::iterator it;
	for (it = usernames_set.begin(); it != usernames_set.end(); ++it)
	{	
		string user = *it;
		if (!user.compare(username)){
			usernames_set.erase(it);
			break;
		}
	}
}

/**
 * Disconnect a user
*/
void Server::disconnect_user(int fd)
{	
	if (user_exists(fd))
	{
		string username = usernames.at(fd);
		cout << username <<  " has left" << endl;
		// buffer_content.set_body(username + " has left the chat");
		string message = username + " has left the chat";
		send_to_all(message);
		usernames.erase(fd);
		remove_from_set(username);
	}
	FD_CLR(fd, &active_set);
	close(fd);
}

void Server::service_tcp_client_request(int fd)
{
	struct sockaddr_in address;
	int client_fd = accept_connection(fd, address);
	string welcome_message = "Welcome, type HELP for available commands\n";
	write_to_fd(client_fd, welcome_message);
	FD_SET(client_fd, &active_set);
	update_max_fd(client_fd);
}

/**
 * SERVER CONNECTION RELATED METHODS
 * Below methods are related to connected servers
*/


/**
 * Check if we find server by port
*/
bool Server::is_server_in_list(int port)
{
	for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		if (it->second.get_port() == port)
		{
			return true;
		}
	}
	return false;
}


/**
 * Check if the server id is our neighbor
*/
bool Server::is_neighbor(string server_id)
{
	for (auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		if (!it->second.get_id().compare(server_id))
		{
			return true;
		}
	}
	return false;
}

/**
 * Check if the FD is a neighbor
*/
bool Server::is_neighbor(int fd)
{
	if(neighbors.count(fd) == 1)
	{
		return true;
	}
	return false;
}


/**
 * List all neighbour servers
*/
string Server::listservers()
{
	string servers;
	if (neighbors.empty())
	{
		servers = "Server " + get_id() + " has no neighbours\n";
	}
	else 
	{
		for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
		{
			stringstream ss;
			ss << it->second.get_port();
			servers += it->second.get_id() + "," + it->second.get_host() + "," + ss.str() + ";";
		}
	}
	return servers;
}


/**
 * Add neighbor server to serverlist
*/
void Server::add_to_serverlist(int fd, struct sockaddr_in& address, string server_id)
{
	FD_SET(fd, &active_set);
	update_max_fd(fd);
	stringstream ss;
	ss << ntohs(address.sin_port);
	ServerConnection server_connection = ServerConnection();
	server_connection.set_fd(fd);
	server_connection.set_id(server_id);
	server_connection.set_host(string(inet_ntoa(address.sin_addr)));
	server_connection.set_port(ntohs(address.sin_port));
	pair<int, ServerConnection> pair;
	pair.first = fd;
	pair.second = server_connection;
	neighbors.insert(pair);
	neighbor_connections++;
}

/**
 * Update a neighbor's id, ip and port
*/
void Server::update_neighbor(int fd, string id, string ip, int port)
{
	for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		if(it->first == fd)
		{
			it->second.set_id(id);
			it->second.set_ip(ip);
			it->second.set_port(port);
		}
	}
}

/**
 * Update a neighbor's id
*/
void Server::update_neighbor_id(int fd, string new_id)
{
	for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		if (it->first == fd)
		{
			if (!it->second.get_id().compare("anonymous")){
				it->second.set_id(new_id);
			}
		}
	}
}


/**
 * Accept incomming server connection from another server
 * If maximium neighbors exceeded, then we do not accept
*/
void Server::accept_incomming_server(int fd, struct sockaddr_in& address)
{

	if(neighbor_connections < MAX_NEIGHBOUR_CONNECTIONS)
	{
		// send ID to incomming server
		// FD_SET(fd, &active_set);
		// update_max_fd(fd);
		string server_id = "anonymous";
		// string server_id = retreive_id(fd);
		string request_id = "CMD,," + get_id() + ",ID";
		request_id = string_utilities::wrap_with_tokens(request_id);
		add_to_serverlist(fd, address, server_id);
		write_to_fd(fd, request_id);
	}
	else 
	{
		string message = "Server has maximized allowed connections\n";
		write_to_fd(fd, message);
		FD_CLR(fd, &active_set);
		close(fd);
	}
}


/**
 * Connect the server to another server
*/
void Server::connect_to_server(string sub_command)
{
	// Check if we have an open slot
	if (neighbor_connections == MAX_NEIGHBOUR_CONNECTIONS)
	{
		printf("Server has reached maximum neighbor threshold\n");
		return;
	}

	int n, fd;
	char buffer[32];
	memset(buffer, 0, 32);
	string server_id = "anonymous";
	vector<string> host_and_port = string_utilities::split_by_delimeter(sub_command, ":");
	// Check for correctness of ip:port format of argument
	if (host_and_port.size() < 2) { return; }
	// check if port is a number
	if (!string_utilities::is_number(host_and_port.at(1))) { return; }

	string host = host_and_port.at(0);
	int port = stoi(host_and_port.at(1));
	// prevent connecting to ourselves
	// TODO: remove magic strings localhost and 127.0.0.1
	if (port == ntohs(server_conn_port.second.sin_port) && (!host.compare("localhost") || !host.compare("127.0.0.1")))
	{
		printf("Cannot connect to oneself mate :)\n");
		return;
	}
	// check if it is already our neighbor
	if (is_server_in_list(port)) { return; }
	// setup connection arguments
	fd = socket_utilities::create_tcp_socket(false);
	struct sockaddr_in address;
	struct hostent* h;
	h = gethostbyname(host.c_str());
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	memcpy((char*)&address.sin_addr.s_addr, (char*) h->h_addr, h->h_length);
	// connect
	n = connect(fd, (struct sockaddr*) &address, sizeof(address));
	if (n < 0)
	{
		printf("failed to establish connection to %s:%d\n", host.c_str(), port);
		close(fd);
		return;
	}
	// FD_SET(fd, &active_set);
	// update_max_fd(fd);

	printf("Connection established to server %s:%d with fd %d\n", host.c_str(), port, fd);

	// prepare and send the ID command to server
	string request_id = "CMD,," + get_id() + ",ID";
	request_id = string_utilities::wrap_with_tokens(request_id);
	add_to_serverlist(fd, address, server_id);
	write_to_fd(fd, request_id);
}

/**
 * Service the server TCP request
*/
void Server::service_tcp_server_request(int fd)
{
	struct sockaddr_in address;
	int server_fd = accept_connection(fd, address);
	accept_incomming_server(server_fd, address);
}

/**
 * Get the neighbor file descriptor by it's id
*/
int Server::get_neighbor_fd_by_id(string id)
{
	for (auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		if(!it->second.get_id().compare(id))
		{
			return it->second.get_fd();
		}
	}
	return -1;
}

/*
 *	Disconnect a server
*/
void Server::disconnect_server(int fd)
{
	if (neighbors.find(fd) != neighbors.end())
	{
		close(fd);
		FD_CLR(fd, &active_set);
		cout << "server " << neighbors.at(fd).get_id() << " has disconnected." << endl;
		routing_table.remove(neighbors.at(fd).get_id());
		neighbors.erase(fd);
		neighbor_connections--;
	}
}

/**
 * CLIENT/SERVER CONNECTION RELATED METHODS
 * Below methods relate to client and server connections equallt
*/

/**
 * Write message to a socket
*/
void Server::write_to_fd(int fd, string message)
{
	if (socket_utilities::write_to_fd(fd, message) < 0)
	{
		cout << "ERRNO: " << errno << endl;
		if (!(errno == EWOULDBLOCK || errno == EAGAIN))
		{
			// FD_CLR(fd, &active_set);
			// close(fd);
			if(is_neighbor(fd))
			{
				cout << "disconnecting server" << endl;
				disconnect_server(fd);
			}
			else
			{
				disconnect_user(fd);
			}
		}
	}
}

/**
 * Update the maximum file descriptor variable
*/
void Server::update_max_fd(int fd)
{
	if (fd > max_file_descriptor) {
		max_file_descriptor = fd;
	}
}


/**
 * Accept a connection
*/
int Server::accept_connection(int fd, struct sockaddr_in& address)
{

	socklen_t address_length = sizeof(address);
	int new_socket = accept(fd, (struct sockaddr *)&address, &address_length);
	if(new_socket < 0){
		socket_utilities::error("Failed to establish connection");
	}
	printf("Connection established from %s port %d and fd %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port), new_socket);	
	return new_socket;
}

/**
 * Wrapper method for select
*/
void Server::select_wrapper(fd_set& set)
{
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	if (select(max_file_descriptor + 1, &set, NULL, NULL, &tv) < 0)
	{
		printf("Errno: %d\n", errno);
		if (errno == EBADF || errno == EAGAIN)
		{

		}
	}
}

/**
 * Service the udp request
*/
void Server::service_udp_request(int fd)
{
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	struct sockaddr_in udp_sender;
	socklen_t sender_length = sizeof(udp_sender);
	int read_bytes = recvfrom(udp_port.first, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &udp_sender, &sender_length);
	if (read_bytes > 0)
	{
		string servers = listservers();
		int n = sendto(fd, servers.c_str(), servers.length(), 0, (struct sockaddr*) &udp_sender, sizeof(udp_sender));
		if (n < 0)
		{
			
		}
	}
}

/**
 * receive wrapper
*/
void Server::receive_from_client_or_server(int fd)
{
	vector<string> vector_buffer;
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	int read_bytes = recv(fd, buffer, MAX_BUFFER_SIZE, 0);
	if (read_bytes < 0)
	{
		if (!(errno == EWOULDBLOCK || errno == EAGAIN))
		{
			FD_CLR(fd, &active_set);
			close(fd);
		}
	}
	/* client/server disconnects friendly without using the LEAVE command*/
	else if (read_bytes == 0)
	{
		if (is_neighbor(fd))
		{
			// REMOVE SERVER
			disconnect_server(fd);
		}
		else 
		{
			string feedback_message;
			disconnect_user(fd);
		}
	}

	else 
	{
		string buff = string(buffer);
		if (is_neighbor(fd))
		{
			if (!(buff[0] == SOH) && !(buff[buff.size() - 1] == EOT))
			{
				return;
			}			
			// Remove bytestuffed tokens
			buff = buff.substr(1, buff.size() - 2);
			string_utilities::trim_string(buff);
		}
		execute_command(fd, buff, "");
	}
}


/**
 * Execute a command
 * Parses the buffer and assigns commands / sub commands and messages to variables
 * Then it checks which command to execute and executes it
*/
void Server::execute_command(int fd, string buffer, string from_server_id)
{	
	string feedback_message = "";
	string command = "";
	string sub_command = "";
	string body = "";
	string source_server_id = "";
	string destination_server_id = "";
	string CMD_command = "";
	string RSP_response = "";
	vector<string> vector_buffer;

	vector<string> check_first = string_utilities::split_by_delimeter(buffer, ",");
	string first_command = check_first.size() > 0 ? check_first.at(0) : "";
	if (!first_command.compare("CMD") || !first_command.compare("RSP"))
	{
		// NOT IN USE
		if (check_first.size() >= 4)
		{
			vector_buffer = string_utilities::split_by_delimeter_stopper(buffer, ",", 3);
			command = vector_buffer.at(0);
			destination_server_id = vector_buffer.at(1);
			source_server_id = vector_buffer.at(2);
			if (!first_command.compare("CMD"))
			{
				CMD_command = vector_buffer.at(3);
			}
			else if (!first_command.compare("RSP"))
			{
				RSP_response = vector_buffer.at(3);
			}
		}

	}
	else if (!first_command.compare("LEAVE") || !first_command.compare("WHO") || !first_command.compare("ID") || !first_command.compare("LISTSERVERS") || !first_command.compare("LISTROUTES") || !first_command.compare("KEEPALIVE"))
	{
		vector_buffer = string_utilities::split_by_delimeter_stopper(buffer, ",", 1);
		command = vector_buffer.at(0);
	}
	else if (!first_command.compare("FETCH") || !first_command.compare("CS") || !first_command.compare("CONNECT"))
	{
		if (check_first.size() == 2)
		{
			vector_buffer = string_utilities::split_by_delimeter_stopper(buffer, ",", 2);
			command = vector_buffer.at(0);
			sub_command = vector_buffer.at(1);
		}
	}
	else if (!first_command.compare("MSG"))
	{
		if (check_first.size() >= 3)
		{
			vector_buffer = string_utilities::split_by_delimeter_stopper(buffer, ",", 2);
			command = vector_buffer.at(0);
			sub_command = vector_buffer.at(1);
			body = vector_buffer.at(2);
		}
	}

	// Execute the actual command
	if ((!command.compare("CS")))
	{
		connect_to_server(sub_command);
	}

	/* Client/Server Commands */
	else if (!command.compare("LISTSERVERS"))
	{
		if (is_neighbor(fd))
		{	
			// TODO:: check for tokens and remove
			string RSP = "RSP," + from_server_id + "," + get_id() + ",LISTSERVERS," + listservers();
			RSP = string_utilities::wrap_with_tokens(RSP);
			write_to_fd(fd, RSP);
		}
		else { write_to_fd(fd, listservers()); }
	}

	else if (!command.compare("KEEPALIVE"))
	{
		// TODO: update keepalive stopwatch for sending server		
	}
	
	else if (!command.compare("LISTROUTES"))
	{
		if (is_neighbor(fd))
		{
			string RSP = "RSP," + from_server_id + "," + get_id() + ",LISTROUTES," + routing_table.to_string();
			RSP = string_utilities::wrap_with_tokens(RSP);
			write_to_fd(fd, RSP);
		}
		else { write_to_fd(fd, routing_table.to_string()); }
	}

	else if (!command.compare("FETCH"))
	{

		if(!string_utilities::is_number(sub_command)) { return; }
		int index = stoi(sub_command);
		if(index < 1 || index > 5) { return; }
		if (is_neighbor(fd))
		{
			string RSP = "RSP," + from_server_id + "," + get_id() + ",FETCH," + hashes.at(index);
			RSP = string_utilities::wrap_with_tokens(RSP);
			write_to_fd(fd, RSP);
		}
		else 
		{ 
			write_to_fd(fd, hashes.at(index)); 
		}
	}

	else if (!command.compare("CMD"))
	{
		if (!(destination_server_id.compare(get_id())) || !(destination_server_id.compare("")))
		{
			update_neighbor_id(fd, source_server_id);
			// vector<string> new_buffer = parse_buffer(CMD_command, fd);
			execute_command(fd, CMD_command, source_server_id);
		}
		// We are not the recipient, forward
		
		else
		{
			for(host_pair_t pair : routing_table.get_hosts())
			{	
				if (!destination_server_id.compare(pair.first))
				{
					if (is_neighbor(pair.second))
					{
						int fd = get_neighbor_fd_by_id(pair.second);
						if (fd > 0)
						{
							string forwarding = string_utilities::wrap_with_tokens(buffer);
							write_to_fd(fd, forwarding);
						}
					}
				}
			}
		}		
	}
	else if (!command.compare("RSP"))
	{
		vector<string> RSP_response_vector = string_utilities::split_by_delimeter_stopper(RSP_response, ",", 1);
		if (RSP_response_vector.size() < 2) { return; }
		string response_command = RSP_response_vector.at(0);
		string response = RSP_response_vector.at(1);

		if (!destination_server_id.compare(get_id()))
		{
			cout << "returned response: " << response << " from server " << source_server_id << endl;
			if(!response_command.compare("ID"))
			{	
				vector<string> id_ip_port = string_utilities::split_by_delimeter(response, ",");
				if (id_ip_port.size() == 3)
				{
					string id = id_ip_port.at(0);
					string ip = id_ip_port.at(1);
					int port = stoi(id_ip_port.at(2));
					update_neighbor(fd, id, ip, port);
					routing_table.add(string_utilities::trim_string(source_server_id), id);
					// cout << routing_table.to_string() << endl;
				}
			}

			else if (!response_command.compare("LISTSERVERS"))
			{
				vector<string> serverlist = string_utilities::split_by_delimeter(response, ";");
				for(int i = 0; i < serverlist.size(); ++i)
				{
					vector<string> server = string_utilities::split_by_delimeter(serverlist.at(i), ",");
					if (server.size() == 3)
					{
						routing_table.add(string_utilities::trim_string(source_server_id), server.at(0));
					}
				}
				// cout << routing_table.to_string() << endl;
			}

			else if (!response_command.compare("FETCH"))
			{

			}
		}
		// We are not the destination
		else {
			for(host_pair_t pair : routing_table.get_hosts())
			{	
				if (!destination_server_id.compare(pair.first))
				{
					if (is_neighbor(pair.second))
					{
						int fd = get_neighbor_fd_by_id(pair.second);
						if (fd > 0)
						{
							string forwarding = string_utilities::wrap_with_tokens(buffer);
							write_to_fd(fd, forwarding);
						}
					}
				}
			}
		}
	}

	else if (!command.compare("ID"))
	{
		// If it is a server requesting the ID, we respond with RSP
		if (is_neighbor(fd))
		{
			stringstream ss;
			ss << ntohs(server_conn_port.second.sin_port);
			// TODO: Change public IP address if to run on on skel!
			string RSP = "RSP," + from_server_id + "," + get_id() + ",ID," + get_id() + ",89.160.128.13," + ss.str();
			RSP = string_utilities::wrap_with_tokens(RSP);
			write_to_fd(fd, RSP);
		}
		// Client requesting ID
		else { write_to_fd(fd, get_id()); }
		
	}

	else if (!command.compare("CONNECT")) 
	{	
		cout << sub_command << " has connected" << endl;
		if (add_user(fd, sub_command, feedback_message))
		{
			// write to all
			// buffer_content.set_body(feedback_message);
			send_to_all(feedback_message);
		}
		else
		{
			write_to_fd(fd, feedback_message);
		}

	}

	else if (!command.compare("LEAVE")) 
	{

		disconnect_user(fd);
	}

	else if (!command.compare("WHO")) 
	{
		display_users(fd);

	}
	else if (!command.compare("MSG")) 
	{
		// sub_command = buffer_content.get_sub_command();
		if (user_exists(fd)){
			string sending_user = usernames.at(fd);
			if (!sub_command.compare("ALL"))
			{
				// send to all
				string message = sending_user + ": " + body;
				send_to_all(message);
			}
			else
			{
				// send to single user
				int rec_fd = get_fd_by_user(sub_command);
				if (rec_fd > 0)
				{	
					// do not send to yourself
					if(fd != rec_fd)
					{	
						string message = sending_user + ": " + body;
						send_to_user(rec_fd, message);
					}
					else
					{
						feedback_message = "Cannot send message to yourself\n";
						write_to_fd(fd, feedback_message);
					}
				}
				// no user found
				else
				{	
					feedback_message = "No such user\n";
					write_to_fd(fd, feedback_message);
				}
			}
		}

		else 
		{
			feedback_message = "You need to be logged in\n";
			write_to_fd(fd, feedback_message);
		}
	
	}

	else if (!command.compare("HELP"))
	{
		display_commands(fd);
	}

	else
	{
		if (!is_neighbor(fd))
		{
			feedback_message = "Unknown command, type HELP for commands\n";
			write_to_fd(fd, feedback_message);
		}
	}

}

/**
 * Send a keepalive to a fd
*/
void Server::send_keepalive(int fd)
{
	auto find_server = neighbors.find(fd);
	if(find_server != neighbors.end())
	{
		ServerConnection* serv = &find_server->second;
		string ser_id = serv->get_id();
		string send_keep = "CMD," + ser_id + "," + get_id() + ",KEEPALIVE";
		send_keep = string_utilities::wrap_with_tokens(send_keep);
		write_to_fd(fd, send_keep);
	}
}

/**
 * Clear the routing table
*/
void Server::clear_routing_table()
{
	routing_table.clear();
	for (auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		routing_table.add(it->second.get_id(), it->second.get_id());
	}
	clear_routes_timer.reset();
}

/**
 * Send keepalive to all neighbors
*/
void Server::send_keepalive_to_all()
{
	for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		send_keepalive(it->first);
	}
	keepalive_timer.reset();
}

/**
 * Update our routing table
*/
void Server::update_routes()
{
	for(host_pair_t pair : routing_table.get_hosts())
	{
		string request_listservers = "CMD," + pair.first + "," + get_id() + ",LISTSERVERS";
		request_listservers = string_utilities::wrap_with_tokens(request_listservers);
		int fd = get_neighbor_fd_by_id(pair.second);
		if (fd > 0)
		{
			write_to_fd(fd, request_listservers);
		}
	}
	routing_table_timer.reset();
}

/**
 * Print the routing table
*/
void Server::print_routing_status()
{
	cout << routing_table.to_string();
	print_routes_timer.reset();
}

/**
 * Runs all scheduled tasks
*/
void Server::run_scheduled_tasks()
{
	if(keepalive_timer.elapsed(60))
	{
		send_keepalive_to_all();
	}

	if(routing_table_timer.elapsed(30))
	{
		update_routes();
		
	}
	if(clear_routes_timer.elapsed(60*2))
	{
		clear_routing_table();
	}
	if(print_routes_timer.elapsed(60))
	{
		print_routing_status();
	}
}


/**
 * PUBLIC METHODS
*/

/**
 * Run the server
*/
int Server::run()
{
	/* buffer for messages */
	printf("Listening for client connections on port %d\n", ntohs(client_conn_port.second.sin_port));
	printf("Listening for server connections on port %d\n", ntohs(server_conn_port.second.sin_port));
	printf("Listening for udp connections on port %d\n", ntohs(udp_port.second.sin_port));
	keepalive_timer.start();
	routing_table_timer.start();
	clear_routes_timer.start();
	print_routes_timer.start();
	while (1)
	{
		/* copy active_set to read_set to not loose information about active_set status since select alters the set passed as argument */
		fd_set read_set = active_set;

		/* wait for incomming connections */

		select_wrapper(read_set);
		/* Loop from 0 to max FD to act on any read ready file descriptor */
		for (int i = 0; i <= max_file_descriptor; i++)
		{
			if (FD_ISSET(i, &read_set))
			{
				// If a connection is opened on the UDP port.
				if (i == udp_port.first)
				{
					service_udp_request(i);
				}
				// If a connection is open on the server port
				else if (i == server_conn_port.first)
				{
					service_tcp_server_request(i);
				}
				// If a connection is open on the client port
				else if (i == client_conn_port.first)
				{
					service_tcp_client_request(i);
				}
				/* i is some already connected client/server that has send a message */
				else 
				{
					receive_from_client_or_server(i);
				}
			}
		}
		run_scheduled_tasks();
	}
	sleep(1);
	return 0;
}


/**
 * Set the max buffer size
*/
void Server::set_max_buffer(int size)
{
	MAX_BUFFER_SIZE = size;
}


