#include "server.h"

/**
 * Initialize a Server with fortune
*/
Server::Server(int server_p, int client_p, int udp_p, std::string server_id)
{

	id = server_id; // might later take in argv[0] from main and assign
	FD_ZERO(&active_set);
	struct sockaddr_in s;
	struct sockaddr_in c;
	struct sockaddr_in u;

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
}


/**
 * Get the fortune
*/
std::string Server::get_id()
{
	return id;
}


/**
 * Set the max buffer size
*/
void Server::set_max_buffer(int size)
{
	MAX_BUFFER_SIZE = size;
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

void Server::add_to_serverlist(int fd, struct sockaddr_in& address, std::string server_id)
{
	std::stringstream ss;
	ss << ntohs(address.sin_port);
	std::string value = server_id + "," + std::string(inet_ntoa(address.sin_addr)) + "," + ss.str() + ";";
	neighbors_by_fd.insert(Pair(fd, value));
	std::pair<std::string, int> conn_info_pair;
	// conn_info_pair.first = std::string(inet_ntoa(address.sin_addr)) + "," + ss.str();
	// conn_info_pair.second = fd;
	// neighbors_by_conninfo.insert(conn_info_pair);
	neighbor_connections++;
}

void Server::update_server_id(int fd)
{
	for(auto it = neighbors_by_fd.begin(); it != neighbors_by_fd.end(); ++it)
	{

	}
}


/**
 * Accept incomming server connection
*/
void Server::accept_incomming_server(int fd, struct sockaddr_in& address)
{

	if(neighbor_connections < MAX_NEIGHBOUR_CONNECTIONS)
	{
		// send ID to incomming server
		FD_SET(fd, &active_set);
		update_max_fd(fd);
		std::string server_id = "anonymous";
		// std::string server_id = retreive_id(fd);
		add_to_serverlist(fd, address, server_id);
		std::cout << "accept_incomming_connections: " << server_id << std::endl;
	}
	else 
	{
		std::string message = "Server has maximized allowed connections\n";
		write_to_fd(fd, message);
		FD_CLR(fd, &active_set);
		close(fd);
	}
}



/**
 * Connect to a server
*/
void Server::connect_to_server(BufferContent& buffer_content)
{

	if (neighbor_connections == MAX_NEIGHBOUR_CONNECTIONS)
	{
		printf("Server has reached maximum neighbor threshold\n");
		return;
	}

	int n, fd;
	char buffer[32];
	memset(buffer, 0, 32);
	std::string server_id = "anonymous";
	std::string sub_command = buffer_content.get_sub_command();
	std::vector<std::string> host_and_port = string_utilities::split_by_delimeter(sub_command, ":");

	if (host_and_port.size() < 2) { return; }
	if (!string_utilities::is_number(host_and_port.at(1))) { return; }

	std::string host = host_and_port.at(0);
	int port = stoi(host_and_port.at(1));

	if (port == ntohs(server_conn_port.second.sin_port))
	{
		printf("Cannot connect to oneself mate :)\n");
		return;
	}

	// create FD for new connection
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
	FD_SET(fd, &active_set);
	update_max_fd(fd);

	printf("successfully established connection to server %s:%d with fd %d\n", host.c_str(), port, fd);

	std::string request_id = "CMD,," + get_id() + ",ID";
	request_id = string_utilities::wrap_with_tokens(request_id);

	write_to_fd(fd, request_id);
	memset(buffer, 0, 32);
	n = recv(fd, buffer, 32, 0);
	if (n > 0)
	{
		string_utilities::trim_cstr(buffer);
		server_id = std::string(buffer);
	}

	add_to_serverlist(fd, address, server_id);
	std::cout << "connect_to_server: " << server_id << std::endl;

}


/**
 * Display all commands
*/
void Server::display_commands(int fd)
{
	std::string help_message = "\nAvailable commands are:\n\n";
	help_message += "ID\t\tGet the ID of the server\n";
	help_message += "CHANGE ID\t\tChange the ID of the server\n";
	help_message += "CONNECT <username>\tIdentify yourself with username, cannot include space\n";
	help_message += "LEAVE\t\t\tLeave chatroom\n";
	help_message += "WHO\t\t\tGet list of connected users\n";
	help_message += "MSG <user> <message>\tSend a message to specific user\n";
	help_message += "MSG ALL <message>\tSend message to all users connected\n";
	help_message += "HELP\t\t\tSe available commands\n\n";
	write_to_fd(fd, help_message);
}

/**
 * Display all users
*/
void Server::display_users(BufferContent& buffer_content)
{
	int fd = buffer_content.get_file_descriptor();
	std::string users = "\nLIST OF USERS:\n";
	std::set<std::string>::iterator it;
	for (it = usernames_set.begin(); it != usernames_set.end(); ++it){
		std::string username = *it;
		users += " " + username + "\n";
	}
	write_to_fd(fd, users);
}

/**
 * List all neighbour servers
*/
std::string Server::listservers()
{
	std::string servers;
	if (neighbors_by_fd.empty())
	{
		servers = "Server " + get_id() + " has no neighbours\n";
	}
	else 
	{
		// std::map<int, std::string>::iterator it;
		for(auto it = neighbors_by_fd.begin(); it != neighbors_by_fd.end(); ++it)
		{
			servers += it->second;
		}
	}
	return servers;
}


/**
 * Add a newly connected user
*/
bool Server::add_user(BufferContent& buffer_content, std::string& feedback_message)
{
	std::string username = buffer_content.get_sub_command() + buffer_content.get_body();
	int fd = buffer_content.get_file_descriptor();
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
 * Broadcast to all clients
*/
void Server::send_to_all(BufferContent& buffer_content)
{	
	int fd = buffer_content.get_file_descriptor();
	std::string body = buffer_content.get_body();
	for (int i = 0; i <= max_file_descriptor; i++)
	{
		/* Check if the client is in the active set */
		if (FD_ISSET(i, &active_set))
		{
			if (i != server_conn_port.first && i != client_conn_port.first && i != udp_port.first && i != fd)
			{
				write_to_fd(i, body);
			}
		}
	}
}

/**
 * Send a message to a single client
*/
void Server::send_to_user(int rec_fd, BufferContent& buffer_content)
{
	std::string body = buffer_content.get_body();
	if (FD_ISSET(rec_fd, &active_set)){
		write_to_fd(rec_fd, body);
	}
}

/**
 * Remove a user from the set of usernames
*/
void Server::remove_from_set(std::string username)
{
	std::set<std::string>::iterator it;
	for (it = usernames_set.begin(); it != usernames_set.end(); ++it)
	{	
		std::string user = *it;
		if (!user.compare(username)){
			usernames_set.erase(it);
			break;
		}
	}
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

bool Server::is_server(int fd)
{
	if(neighbors_by_fd.count(fd) == 1)
	{
		return true;
	}
	return false;
}

/**
 * Get a file descriptor by username
*/
int Server::get_fd_by_user(std::string username)
{
	int fd = -1;
	std::map<int, std::string>::iterator it;
	for (it = usernames.begin(); it != usernames.end(); ++it)
	{
		std::string user = it->second;
		if (!user.compare(username))
		{		
			fd = it->first;
		}
	}
	return fd;
}

void Server::write_to_fd(int fd, std::string message)
{
	if (socket_utilities::write_to_fd(fd, message) < 0)
	{
		if (!(errno == EWOULDBLOCK || errno == EAGAIN))
		{
			FD_CLR(fd, &active_set);
			close(fd);
		}
	}
}


void Server::disconnect_user(BufferContent& buffer_content)
{	
	int fd = buffer_content.get_file_descriptor();
	if (user_exists(fd))
	{
		std::string username = usernames.at(buffer_content.get_file_descriptor());
		std::cout << username <<  " has left" << std::endl;
		buffer_content.set_body(username + " has left the chat");
		send_to_all(buffer_content);
		usernames.erase(buffer_content.get_file_descriptor());
		remove_from_set(username);
	}
	FD_CLR(fd, &active_set);
	close(fd);
}

void Server::select_wrapper(fd_set& set)
{
	if (select(max_file_descriptor + 1, &set, NULL, NULL, NULL) < 0)
	{
		printf("Errno: %d\n", errno);
		if (errno == EBADF || errno == EAGAIN)
		{

		}
	}
}

void Server::service_udp_request(int fd)
{
	char buffer[MAX_BUFFER_SIZE];
	memset(buffer, 0, MAX_BUFFER_SIZE);
	struct sockaddr_in udp_sender;
	socklen_t sender_length = sizeof(udp_sender);
	int read_bytes = recvfrom(udp_port.first, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*) &udp_sender, &sender_length);
	if (read_bytes > 0)
	{
		std::string servers = listservers();
		int n = sendto(fd, servers.c_str(), servers.length(), 0, (struct sockaddr*) &udp_sender, sizeof(udp_sender));
		if (n < 0)
		{
			
		}
	}
}

void Server::service_tcp_server_request(int fd)
{
	struct sockaddr_in address;
	int server_fd = accept_connection(fd, address);
	accept_incomming_server(server_fd, address);
}

void Server::service_tcp_client_request(int fd)
{
	struct sockaddr_in address;
	int client_fd = accept_connection(fd, address);
	std::string welcome_message = "Welcome, type HELP for available commands\n";
	write_to_fd(client_fd, welcome_message);
	FD_SET(client_fd, &active_set);
	update_max_fd(client_fd);
}

void Server::receive_from_client_or_server(int fd)
{
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
	/* client disconnects friendly without using the LEAVE command*/
	else if (read_bytes == 0)
	{
		///TODO: Check if server is leaving and remove from server structures
		if (is_server(fd))
		{
			// REMOVE SERVER
			neighbors_by_fd.erase(fd);
		}
		else 
		{
			BufferContent buffer_content;
			std::string feedback_message;
			buffer_content.set_file_descriptor(fd);
			disconnect_user(buffer_content);
		}
	}
	else 
	{
		std::string buff = std::string(buffer);
		if (is_server(fd))
		{
			//TODO: Check for SOH and EOT tokens
			//		Parse buffer and remove tokens
			//		Check if CMD or RSP
			//		If to == our_id then parse_buffer(COMMAND)
			//		Else try to find out where to forward.
			if (!(buff[0] == SOH) && !(buff[buff.size() - 1] == EOT))
			{
				return;
			}			
			buff = buff.substr(1, buff.size() - 2);
			string_utilities::trim_string(buff);
		}
		parse_buffer(buff, fd);
	}
}

//TODO: not finished, manual command to fetch ID from a server by FD
void Server::fetch_id_from_fd(int fd)
{
	char buffer[32];
	memset(buffer, 0, 32);
	write_to_fd(fd, "ID");
	int n = recv(fd, buffer, 32, 0);
	if (n > 0)
	{
		string_utilities::trim_cstr(buffer);
		std::string server_id = std::string(buffer);
		// UPDATE SERVER LIST 
		for(auto it = neighbors_by_fd.begin(); it != neighbors_by_fd.end(); ++it)
		{
			if (it->first == fd)
			{
				// anonymous,127.0.0.1,4001
				// std::string value = it->second;
				std::vector<std::string> vec = string_utilities::split_by_delimeter(it->second, ",");
				it->second = server_id + "," + vec.at(1) + "," + vec.at(2) + ";";
			}
		}
		// update_server_list(std::string(buffer));
	}

	for(auto it = neighbors_by_fd.begin() ; it != neighbors_by_fd.end(); ++it)
	{
		std::cout << it->second << ", ";
	}
}



/**
 * Execute a command
*/
void Server::execute_command(BufferContent& buffer_content)
{	
	std::string command = buffer_content.get_command();
	std::string sub_command;
	std::string feedback_message;
	int fd = buffer_content.get_file_descriptor();

	std::cout << buffer_content.get_file_descriptor() << std::endl;
	std::cout << buffer_content.get_command() << std::endl;
	std::cout << buffer_content.get_sub_command() << std::endl;
	std::cout << buffer_content.get_body() << std::endl;
	/* C&C commands */
	if ((!command.compare("CS")))
	{
		connect_to_server(buffer_content);
	}

	//TODO: Finish implementing manual fetch id 
	else if (!command.compare("FETCHID"))
	{
		std::cout << command << std::endl;
		sub_command = buffer_content.get_sub_command();
		std::cout << sub_command << std::endl;
		// struct hostent* h;
		// std::vector<std::string> host_and_port = string_utilities::split_by_delimeter(sub_command, ",");
		// if (host_and_port.size() == 2)
		// {
		// 	h = gethostbyname(host_and_port.at(0).c_str());
		// 	std::cout << std::string(h->h_addr) << std::endl;

		// }
		// int fd = get_fd_by_host_and_port(string_utilities::trim_string(sub_command));
		fetch_id_from_fd(fd);
	}
	
	/* Client/Server Commands */
	else if (!command.compare("LISTSERVERS"))
	{
		//TODO: Send listservers to FD
		std::string servers = listservers();
		write_to_fd(fd, servers);
	}

	else if (!command.compare("KEEPALIVE"))
	{
		//TODO: Send keepalive to FD
	}
	
	else if (!command.compare("LISTROUTES"))
	{
		//TODO:: send routing table to FD
	}

	else if (!command.compare("FETCH"))
	{
		//TODO:: send Id with number matching stoi(sub_command)
	}

	else if (!command.compare("CMD"))
	{
		std::vector<std::string> source_and_cmd = string_utilities::split_by_delimeter(buffer_content.get_body(), ",");
		std::string source = source_and_cmd.at(0);
		std::string dest = buffer_content.get_sub_command();
		std::string cmd = source_and_cmd.at(1);
		
		if (!(dest.compare(get_id())) || !(dest.compare("")))
		{
			// TODO: find FD in our neighbor map and update server_id for that FD

			// execute the cmd requested
			parse_buffer(cmd, fd);
		}
		else
		{
			// TODO: Implement
			//		 Forward command: CMD,toserver,fromserver,COMMAND
			printf("Find best way to forward command\n");
		}


		
	}
	else if (!command.compare("RSP"))
	{
		//TODO: check if RSP is to go to our neighbor, then write_to_fd(neighborFD, command_response)
		//		else make best assumption on where to forward the resposne
	}

	else if ((!command.compare("ID"))) 
	{
		// If the receiver is a server, it shall RSP the result of the command
		if (is_server(fd))
		{	
			//TODO:	Create a global variable which holds info about current to_server_id
			//		Execute RSP,to_server_id,us,get_id()
			//		Check if to_server_id is neighbor, else find best suitable option to forward response
			std::string response = "RSP,";
		}
		else { write_to_fd(fd, id + "\n"); }
		
	}

	else if ((!command.compare("CONNECT"))) 
	{	
		std::cout << buffer_content.get_sub_command() + buffer_content.get_body() + " has connected" << std::endl;
		if (add_user(buffer_content, feedback_message))
		{
			// write to all
			buffer_content.set_body(feedback_message);
			send_to_all(buffer_content);
		}
		else
		{
			write(fd, feedback_message.c_str(), feedback_message.length());
		}

	}

	else if ((!command.compare("LEAVE"))) 
	{
		disconnect_user(buffer_content);
	}

	else if ((!command.compare("WHO"))) 
	{
		display_users(buffer_content);

	}
	else if ((!command.compare("MSG"))) 
	{
		sub_command = buffer_content.get_sub_command();
		if (user_exists(fd)){
			std::string sending_user = usernames.at(fd);
			if (!sub_command.compare("ALL"))
			{
				// send to all
				buffer_content.set_body(sending_user + ": " + buffer_content.get_body() + "\n");
				send_to_all(buffer_content);
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
						buffer_content.set_body(sending_user + ": " + buffer_content.get_body() + "\n");
						send_to_user(rec_fd, buffer_content);
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

	else if ((!command.compare("HELP")))
	{
		display_commands(buffer_content.get_file_descriptor());
	}

	else
	{
		// Only respond with unknown command message to chat clients. Servers that might 
		// request ID upon connecting to us might cause infinite loop of these messages back and forth.
		if (!is_server(fd))
		{
			feedback_message = "Unknown command, type HELP for commands\n";
			write_to_fd(fd, feedback_message);
		}
	}

}

/**
 * parse the input buffer from the client and execute each command
*/
void Server::parse_buffer(std::string buffer, int fd)
{
	/* split input string by \ to get a vector of commands */
	std::string delimeter = "\\";
	std::vector<std::string> vector_buffer = string_utilities::split_by_delimeter(std::string(buffer), delimeter);
	
	/* for each command, assign variables to buffer_content and execute command */
	for (int i = 0; i < vector_buffer.size(); ++i)
	{
		BufferContent buffer_content;
		buffer_content.set_file_descriptor(fd);
		std::vector<std::string> commands = string_utilities::split_into_commands_and_body(vector_buffer.at(i));
		for (int j = 0; j < commands.size(); ++j)
		{
			std::string cmd = commands.at(j);
			if (j == 0)
			{
				buffer_content.set_command(string_utilities::trim_string(cmd));
			}
			else if (j == 1)
			{	
				buffer_content.set_sub_command(string_utilities::trim_string(cmd));
			}
			else if (j == 2)
			{
				buffer_content.set_body(string_utilities::trim_string(cmd));
			}
		}
		execute_command(buffer_content);
	}
}




/**
 * Run the server
*/
int Server::run()
{
	/* buffer for messages */
	printf("Listening for client connections on port %d\n", ntohs(client_conn_port.second.sin_port));
	printf("Listening for server connections on port %d\n", ntohs(server_conn_port.second.sin_port));
	printf("Listening for udp connections on port %d\n", ntohs(udp_port.second.sin_port));
	
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
	}
	return 0;
}



