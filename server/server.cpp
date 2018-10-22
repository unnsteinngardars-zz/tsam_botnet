#include "server.h"

/**
 * Initialize a Server with fortune
*/
Server::Server(int server_p, int client_p, int udp_p, string server_id)
{
	hashes.insert(Pair(1, "398e3848552c55d931d7ae4b48ffea40"));
	hashes.insert(Pair(2, "a181a603769c1f98ad927e7367c7aa51"));
	hashes.insert(Pair(3, "d80ea254f1b207e19040e7932e958d1c"));
	hashes.insert(Pair(4, "6864f389d9876436bc8778ff071d1b6c"));
	hashes.insert(Pair(5, "ca23ba209cc33678530392b7197fda4d"));

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
string Server::get_id()
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

void Server::add_to_serverlist(int fd, struct sockaddr_in& address, string server_id)
{
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

void Server::update_server_id(int fd, string new_id)
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

void Server::update_server_port(int fd, int port)
{
	for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
	{
		if (it->first == fd)
		{
			it->second.set_port(port);
		}
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
		string server_id = "anonymous";
		// string server_id = retreive_id(fd);
		add_to_serverlist(fd, address, server_id);
		cout << "accept_incomming_connections: " << server_id << endl;
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
 * Connect to a server
*/
void Server::connect_to_server(string sub_command)
{

	if (neighbor_connections == MAX_NEIGHBOUR_CONNECTIONS)
	{
		printf("Server has reached maximum neighbor threshold\n");
		return;
	}

	int n, fd;
	char buffer[32];
	memset(buffer, 0, 32);
	string server_id = "anonymous";
	// string sub_command = buffer_content.get_sub_command();
	vector<string> host_and_port = string_utilities::split_by_delimeter(sub_command, ":");

	if (host_and_port.size() < 2) { return; }
	if (!string_utilities::is_number(host_and_port.at(1))) { return; }

	string host = host_and_port.at(0);
	int port = stoi(host_and_port.at(1));

	if (port == ntohs(server_conn_port.second.sin_port))
	{
		printf("Cannot connect to oneself mate :)\n");
		return;
	}

	//TODO: Check if 

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

	stringstream ss;
	ss << ntohs(server_conn_port.second.sin_port);
	string request_id = "CMD,," + get_id() + ",ID," + ss.str() + " ";
	request_id = string_utilities::wrap_with_tokens(request_id);

	write_to_fd(fd, request_id);
	// memset(buffer, 0, 32);
	// n = recv(fd, buffer, 32, 0);
	// if (n > 0)
	// {
	// 	string_utilities::trim_cstr(buffer);
	// 	server_id = string(buffer);
	// }

	add_to_serverlist(fd, address, server_id);
	cout << "connect_to_server: " << server_id << endl;

}


/**
 * Display all commands
*/
void Server::display_commands(int fd)
{
	string help_message = "\nAvailable commands are:\n\n";
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
	if(neighbors.count(fd) == 1)
	{
		return true;
	}
	return false;
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

void Server::write_to_fd(int fd, string message)
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
		string servers = listservers();
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
	string welcome_message = "Welcome, type HELP for available commands\n";
	write_to_fd(client_fd, welcome_message);
	FD_SET(client_fd, &active_set);
	update_max_fd(client_fd);
}

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
	/* client disconnects friendly without using the LEAVE command*/
	else if (read_bytes == 0)
	{
		///TODO: Check if server is leaving and remove from server structures
		if (is_server(fd))
		{
			// REMOVE SERVER
			neighbors.erase(fd);
			neighbor_connections--;
		}
		else 
		{
			// BufferContent buffer_content;
			string feedback_message;
			// buffer_content.set_file_descriptor(fd);
			disconnect_user(fd);
		}
	}
	else 
	{
		string buff = string(buffer);
		if (is_server(fd))
		{
			// Remove tokens
			if (!(buff[0] == SOH) && !(buff[buff.size() - 1] == EOT))
			{
				return;
			}			
			buff = buff.substr(1, buff.size() - 2);
			string_utilities::trim_string(buff);
		}
		vector_buffer = parse_buffer(buff, fd);
		execute_command(fd, vector_buffer);
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
		string server_id = string(buffer);
		// UPDATE SERVER LIST 
		for(auto it = neighbors.begin(); it != neighbors.end(); ++it)
		{
			if (it->first == fd)
			{
				// anonymous,127.0.0.1,4001
				// string value = it->second;
				// vector<string> vec = string_utilities::split_by_delimeter(it->second, ",");
				// it->second = server_id + "," + vec.at(1) + "," + vec.at(2) + ";";
			}
		}
		// update_server_list(string(buffer));
	}
}

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
 * Execute a command
*/
void Server::execute_command(int fd, vector<string> buffer, string from_server_id)
{	
	string feedback_message = "";
	string command = "";
	string sub_command = "";
	string body = "";
	string source_server_id = "";
	string destination_server_id = "";
	string CMD_command = "";
	string RSP_response = "";
	
	if (is_server(fd))
	{
		cout << "SERVER\n";
		command = (!buffer.empty()) ? buffer.at(0) : "";
		cout << "command: " + command << endl;
		if(!command.compare("ID"))
		{	
			sub_command = (buffer.size() > 1) ? buffer.at(1) : "";
			cout << "sub_command: " << sub_command << endl;
		} else{
			destination_server_id = (buffer.size() > 1) ? buffer.at(1) : "";
		}
		
		cout << "source_server_id :" + source_server_id << endl;
		source_server_id = (buffer.size() > 2) ? buffer.at(2) : "";
		cout << "destination_server_id : " + destination_server_id << endl;
		if(!command.compare("CMD")){
			CMD_command = (buffer.size() > 3) ? buffer.at(3) : "";
			cout << "CMD_command: " + CMD_command << endl;
		}
		else {
			RSP_response = (buffer.size() > 3) ? buffer.at(3) : "";
			cout << "RSP_response: " + RSP_response << endl;
		}
	}
	else {
		cout << "CLIENT\n";
		command = (!buffer.empty()) ? buffer.at(0) : "";
		cout << "command: " + command << endl;
		sub_command = (buffer.size() > 1) ? buffer.at(1) : "";
		cout << "sub_command: " + sub_command << endl;
		body = (buffer.size() > 2) ? buffer.at(2) : "";
		cout << "body: " + body << endl;
		if (!command.compare("CMD") || !command.compare("RSP"))
		{
			destination_server_id = sub_command;
			cout << "destination_server_id : " + destination_server_id << endl;
			vector<string> body_split = string_utilities::split_by_delimeter_stopper(body, ",", 1);
			if (body_split.size() > 1)
			{
				source_server_id = body_split.at(0);
				cout << "source_server_id :" + source_server_id << endl;
				if (!command.compare("CMD"))
				{
					CMD_command = body_split.at(1);
					cout << "CMD_command: " + CMD_command << endl;
				}
				else if (!command.compare("RSP"))
				{
					RSP_response = body_split.at(1);
					cout << "RSP_response: " + RSP_response << endl;
				}
			}
			
		}
	}
	// string command = buffer_content.get_command();
	// string sub_command;
	// string feedback_message;
	// int fd = buffer_content.get_file_descriptor();

	// cout << buffer_content.get_file_descriptor() << endl;
	// cout << buffer_content.get_command() << endl;
	// cout << buffer_content.get_sub_command() << endl;
	// cout << buffer_content.get_body() << endl;
	/* C&C commands */
	if ((!command.compare("CS")))
	{
		connect_to_server(sub_command);
	}

	//TODO: Finish implementing manual fetch id 
	else if (!command.compare("FETCHID"))
	{
		fetch_id_from_fd(fd);
	}
	
	/* Client/Server Commands */
	else if (!command.compare("LISTSERVERS"))
	{
		//TODO: Send listservers to FD
		if (is_server(fd))
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
		//TODO: Send keepalive to FD

	}
	
	else if (!command.compare("LISTROUTES"))
	{
		//TODO:: send routing table to FD
	}

	else if (!command.compare("FETCH"))
	{

		if(!string_utilities::is_number(sub_command)) { return; }
		int index = stoi(sub_command);
		if(index < 1 || index > 5) { return; }
		if (is_server(fd))
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
			update_server_id(fd, source_server_id);		
			vector<string> new_buffer = parse_buffer(CMD_command, fd);
			execute_command(fd, new_buffer, source_server_id);
		}
		// We are not the recipient, forward
		else
		{
			// TODO: Implement
			cout << "Forward this CMD!\n";
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
			if(!response_command.compare("ID"))
			{	
				vector<string> id_port = string_utilities::split_by_delimeter(response, ",");
				if (id_port.size() == 2)
				{
					update_server_id(fd, id_port.at(0));
					update_server_port(fd, stoi(id_port.at(1)));
				}
				else
				{
					update_server_id(fd, response);
				}
			}
			else if (!response_command.compare("LISTSERVERS"))
			{
				// update routing table
			}
		}
		// We are not the destination
		else {
			//TODO: forward shit!
			cout << "Forward this RSP!\n";
		}
	}

	else if ((!command.compare("ID"))) 
	{
		// If it is a server requesting the ID, we respond with RSP
		if (is_server(fd))
		{
			string RSP = "";	
			if(sub_command.compare(""))
			{
				update_server_port(fd, stoi(sub_command));
				stringstream ss;
				ss << ntohs(server_conn_port.second.sin_port);
				RSP = "RSP," + from_server_id + "," + get_id() + ",ID," + get_id() + "," + ss.str();
			}
			else{
				RSP = "RSP," + from_server_id + "," + get_id() + ",ID," + get_id();
			}
			RSP = string_utilities::wrap_with_tokens(RSP);
			write_to_fd(fd, RSP);
		}
		// Client requesting ID
		else { write_to_fd(fd, get_id()); }
		
	}

	else if ((!command.compare("CONNECT"))) 
	{	
		cout << sub_command + body + " has connected" << endl;
		string username = sub_command + body;
		if (add_user(fd, username, feedback_message))
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

	else if ((!command.compare("LEAVE"))) 
	{

		disconnect_user(fd);
	}

	else if ((!command.compare("WHO"))) 
	{
		display_users(fd);

	}
	else if ((!command.compare("MSG"))) 
	{
		// sub_command = buffer_content.get_sub_command();
		if (user_exists(fd)){
			string sending_user = usernames.at(fd);
			if (!sub_command.compare("ALL"))
			{
				// send to all
				// buffer_content.set_body(sending_user + ": " + buffer_content.get_body() + "\n");
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
						// buffer_content.set_body(sending_user + ": " + buffer_content.get_body() + "\n");
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

	else if ((!command.compare("HELP")))
	{
		display_commands(fd);
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

bool Server::response_is_id(string response)
{
	// V_Group_
	string sub = response.substr(0, 7);
	if ( (!sub.compare("V_Group_")) && response.size() < 11)
	{
		return true;
	}
	return false;
}


bool Server::response_is_listservers(string response)
{
	vector<string> servers = string_utilities::split_by_delimeter(response, ";");
	for(int i = 0; i < servers.size(); ++i)
	{
		vector<string> data = string_utilities::split_by_delimeter(servers.at(i), ",");
		if (data.size() != 3)
		{
			return false;
		}
	}
	return true;
}

/**
 * parse the input buffer from the client and execute each command
*/
vector<string> Server::parse_buffer(string buffer, int fd)
{
	vector<string> vector_buffer;
	if (is_server(fd))
	{	
		vector_buffer = string_utilities::split_by_delimeter_stopper(buffer, ",", 3);
	}
	else{
		vector_buffer = string_utilities::split_by_delimeter_stopper(buffer, ",", 2);
		
	}
	return vector_buffer;
	/* split input string by \ to get a vector of commands */
	// string delimeter = "\\";
	// vector<string> commands;
	/* for each command, assign variables to buffer_content and execute command */
	// for (int i = 0; i < vector_buffer.size(); ++i)
	// {
		// BufferContent buffer_content;
		// buffer_content.set_file_descriptor(fd);
		// vector<string> commands = string_utilities::split_by_delimeter_stopper(vector_buffer.at(i), ",", 2);
		// for (int j = 0; j < commands.size(); ++j)
		// {
		// 	string cmd = commands.at(j);
		// 	if (j == 0)
		// 	{
		// 		buffer_content.set_command(string_utilities::trim_string(cmd));
		// 	}
		// 	else if (j == 1)
		// 	{	
		// 		buffer_content.set_sub_command(string_utilities::trim_string(cmd));
		// 	}
		// 	else if (j == 2)
		// 	{
		// 		buffer_content.set_body(string_utilities::trim_string(cmd));
		// 	}
		// }
		
		// execute_command(buffer_content, from_server_id);
		
	// }
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



