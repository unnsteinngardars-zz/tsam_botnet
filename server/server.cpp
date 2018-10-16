#include "server.h"

/**
 * Initialize a Server with fortune
*/
Server::Server()
{
	set_fortune();
}

/**
 * Set the fortune
*/
void Server::set_fortune()
{
	FILE* fp;
	char path[1035];
	std::string fort = "";
	fp = popen("/bin/fortune -s", "r");
	if (fp == NULL){
		printf("No command f");
	}
	while (fgets(path, sizeof(path) - 1, fp) != NULL){
		fort += std::string(path);
	}
	pclose(fp);
	if (fort.compare(""))
	{
		fort += "_Y_Project_2_22_" + time_utilities::get_time_stamp();
		id = fort;
	}
	else{
		id = "NO_ID";
	}
	fprintf(stdout, "fortune: %s\n", id.c_str());
}

/**
 * Get the fortune
*/
std::string Server::get_fortune()
{
	return id;
}

/**
 * Start the knock timer
*/
void Server::set_timer()
{
	time(&knock_start);
}

/**
 * End the knock timer
*/
void Server::stop_timer()
{
	time(&knock_stop);
}

/**
 * Get knock time elapsed in seconds
*/
int Server::get_time_in_seconds()
{
	return difftime(knock_stop, knock_start);
}

/**
 * Set the max buffer size
*/
void Server::set_max_buffer(int size)
{
	MAX_BUFFER_SIZE = size;
}

/**
 * Create server sockets
*/
void Server::create_sockets()
{
	for (int i = 0; i < 3; ++i)
	{
		int socket = socket_utilities::create_socket();
		struct sockaddr_in address;
		servers.push_back(Pair(socket, address));
	}
}

/**
 * Add server sockets to active set
*/
void Server::add_to_active_set()
{
	for (int i = 0; i < servers.size(); ++i)
	{
		FD_SET(servers[i].first, &active_set);
	}
}

/**
 * Listen to server sockets
*/
void Server::listen_to_sockets()
{
	for (int i = 0; i < servers.size(); ++i)
	{
		socket_utilities::listen_on_socket(servers[i].first);
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
int Server::accept_connection(int socket, sockaddr_in& address, socklen_t & length)
{
	int new_socket = accept(socket, (struct sockaddr *)&address, &length);
	if(new_socket < 0){
		socket_utilities::error("Failed to establish connection");
	}
	return new_socket;
}


/**
 * Display all commands
*/
void Server::display_commands(int fd)
{
	// TODO:: ADD ID commands!
	std::string help_message = "\nAvailable commands are:\n\n";
	help_message += "ID\t\tGet the ID of the server\n";
	help_message += "CHANGE ID\t\tChange the ID of the server\n";
	help_message += "CONNECT <username>\tIdentify yourself with username, cannot include space\n";
	help_message += "LEAVE\t\t\tLeave chatroom\n";
	help_message += "WHO\t\t\tGet list of connected users\n";
	help_message += "MSG <user> <message>\tSend a message to specific user\n";
	help_message += "MSG ALL <message>\tSend message to all users connected\n";
	help_message += "HELP\t\t\tSe available commands\n\n";
	write(fd, help_message.c_str(), help_message.length());
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
	write_to_client(fd, users);
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
	usernames.insert(std::pair<int, std::string>(fd, username));
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
			/* prevent the received message from the client from beeing sent to the server and himself */
			if (i != servers.at(0).first && i != servers.at(1).first && i != servers.at(2).first && i != fd)
			{
				write_to_client(i, body);
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
		write_to_client(rec_fd, body);
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

void Server::write_to_client(int fd, std::string message)
{
	if (socket_utilities::write_to_client(fd, message) < 0)
	{
		if (!(errno == EWOULDBLOCK || errno == EAGAIN))
		{
			FD_CLR(fd, &active_set);
			close(fd);
		}
		else if (errno == EBADF)
		{
			printf("Bad file descriptor error for fd: %d\n", fd);
		}
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

	if ((!command.compare("ID"))) 
	{
		printf("client requesting ID\n");
		write_to_client(fd, id + "\n");
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
		if (user_exists(fd))
		{
			std::string username = usernames.at(fd);
			std::cout << username <<  " has left" << std::endl;
			buffer_content.set_body(username + " has left the chat\n");
			send_to_all(buffer_content);
			usernames.erase(fd);
			remove_from_set(username);

		}
		FD_CLR(fd, &active_set);
		socket_utilities::close_socket(fd);


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
				printf("send to all\n");
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
						write_to_client(fd, feedback_message);
					}
				}
				// no user found
				else
				{	
					printf("no user found\n");
					feedback_message = "No such user\n";
					write_to_client(fd, feedback_message);
				}
			}
		}

		else 
		{
			feedback_message = "You need to be logged in\n";
			write(fd, feedback_message.c_str(), feedback_message.length());
		}
	
	}
	else if ((!command.compare("CHANGE"))) 
	{
		sub_command = buffer_content.get_sub_command();
		if( (!sub_command.compare("ID")) )
		{
			printf("change id\n");
			set_fortune();
		}
	}
	else if ( (!command.compare("HELP")) )
	{
		display_commands(buffer_content.get_file_descriptor());
	}

	else
	{
		feedback_message = "Unknown command, type HELP for commands\n";
		write_to_client(fd, feedback_message);
	}

}

/**
 * parse the input buffer from the client and execute each command
*/
void Server::parse_buffer(char * buffer, int fd)
{
	/* Memcopy the buffer onto a local buffer */
	int buffer_length = strlen(buffer);
	char local_buffer[buffer_length];
	memset(local_buffer, 0, buffer_length);
	memcpy(local_buffer, buffer, buffer_length + 1);

	/* split input string by \ to get a vector of commands */
	std::string delimeter = "\\";
	std::vector<std::string> vector_buffer = string_utilities::split_by_delimeter(std::string(local_buffer), delimeter);
	
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
	int MIN_PORT = 49152;
	int MAX_PORT = 65532;
	
	/* zero the active set */
	FD_ZERO(&active_set);

	/* the struct for the incomming client info */
	struct sockaddr_in client;

	/* client_length is used for the accept call */
	socklen_t client_length;

	/* buffer for messages */
	char buffer[MAX_BUFFER_SIZE];

	/* Create 3 sockets/sockaddr_in */
	create_sockets();

	/* find 3 consecutive ports to bind to */
	if (socket_utilities::find_consecutive_ports(MIN_PORT, MAX_PORT, servers) < 0)
	{
		socket_utilities::error("Failed to bind to ports");
	}

	/* listen and add to active set */
	listen_to_sockets();
	add_to_active_set();

	bool knocked_first = false;
	bool knocked_second = false;

	int first_port = ntohs(servers.at(0).second.sin_port);
	int second_port = ntohs(servers.at(1).second.sin_port);
	int third_port = ntohs(servers.at(2).second.sin_port);
	printf("ports %d %d %d\n", first_port, second_port, third_port);
	max_file_descriptor = servers.at(servers.size() - 1).first;

	while (1)
	{
		/* copy active_set to read_set to not loose information about active_set status since select alters the set passed as argument */
		read_set = active_set;
		/* wait for incomming client/s */
		if (select(max_file_descriptor + 1, &read_set, NULL, NULL, 0) < 0)
		{
			if (errno == EBADF)
			{
				printf("Bad file descriptor\n");
			}
			socket_utilities::error("Failed to receive client connection");
		}

		/* Loop from 0 to max FD to act on any read ready file descriptor */
		for (int i = 0; i <= max_file_descriptor; i++)
		{
			if (FD_ISSET(i, &read_set))
			{
				/* if i is the first open port */
				if (i == servers.at(0).first)
				{
					if (!knocked_second)
					{
						set_timer();
						knocked_first = true;
					}
					struct sockaddr_in client;
					socklen_t client_length;
					int client_fd = accept_connection(i, client, client_length);
					close(client_fd);
				}
				/* if i is the second open port */
				else if (i == servers.at(1).first)
				{
					if(knocked_first)
					{
						knocked_second = true;
					}
					struct sockaddr_in client;
					socklen_t client_length;
					int client_fd = accept_connection(i, client, client_length);
					close(client_fd);
				}
				/* if i is the third open port which is the connect port */
				else if (i == servers.at(2).first)
				{
					stop_timer();
					int seconds = get_time_in_seconds();
					/* if time is within 2 seconds and first and second knocks are true then let in */
					if (seconds < 2 && knocked_first && knocked_second) 
					{
						client_length = sizeof(client);
						int client_fd = accept_connection(i, client, client_length);
						printf("Connection established from %s port %d and fd %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), client_fd);
						std::string welcome_message = "Welcome, type HELP for available commands\n";
						write_to_client(client_fd, welcome_message);
						FD_SET(client_fd, &active_set);
						update_max_fd(client_fd);
						knocked_first = false;
						knocked_second = false;

					}
					else 
					{
						struct sockaddr_in client;
						socklen_t client_length;
						int client_fd = accept_connection(i, client, client_length);
						close(client_fd);
						knocked_first = false;
						knocked_second = false;
					}

				}
				/* i is some already connected client that has send a message */
				else 
				{
					memset(buffer, 0, MAX_BUFFER_SIZE);
					int read_bytes = recv(i, buffer, MAX_BUFFER_SIZE, 0);
					if (read_bytes < 0)
					{
						if (!(errno == EWOULDBLOCK || errno == EAGAIN))
						{
							FD_CLR(i, &active_set);
							close(i);
						}
					}
					/* client disconnects friendly without using the LEAVE command*/
					else if (read_bytes == 0)
					{
						BufferContent buffer_content;
						std::string feedback_message;
						buffer_content.set_file_descriptor(i);
						
						if (user_exists(i))
						{
							std::string username = usernames.at(i);
							std::cout << username <<  " has left" << std::endl;
							buffer_content.set_body(username + " has left the chat");
							send_to_all(buffer_content);
							usernames.erase(i);
							remove_from_set(username);
						}
						FD_CLR(i, &active_set);
						socket_utilities::close_socket(i);
					}
					else 
					{
						parse_buffer(buffer, i);
					}
				}

			}
		}
	}
	return 0;
}


void Server::set_scan_destination(std::string dest_host)
{
	destination_server = gethostbyname(dest_host.c_str());

}


void Server::knock_port(int port, char * buffer)
{
	memset(buffer, 0, 1024);
	std::string command = "CONNECT Y_Project_2_22\\ID\\CHANGE ID\\LEAVE";
	int fd = socket_utilities::create_socket();
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);		
	memcpy((char*)&address.sin_addr.s_addr, (char*) destination_server->h_addr, destination_server->h_length);
	int i = socket_utilities::connect(fd, address);
	if (i < 1)
	{
		return;
	}

	else
	{
		int send_bytes = send(fd, command.c_str(), command.length(), 0);
		if (send_bytes < 1)
		{
			return;
		}
		int read_bytes = recv(fd, buffer, 1024, 0);
		if (read_bytes < 1)
		{
			return;
		}
		else
		{	
			printf("port: %d\n", port);
			printf("%s\n", buffer);
			printf("CONNECTED!\n");
			return;
		}
		
	}
}



void Server::scan(int min_port, int max_port, bool scan_when_running)
{
	
	char buffer[1024];
	
	for(int i = min_port; i < max_port; ++i)
	{
		int first_port = i;
		int second_port = i + 1;
		int third_port = i + 2;

		if (scan_when_running)
		{
			if (i != ntohs(servers.at(0).second.sin_port) && i !=  ntohs(servers.at(1).second.sin_port) && i != ntohs(servers.at(2).second.sin_port) && scan_when_running)
			{
				knock_port(first_port, buffer);
				knock_port(second_port, buffer);
				knock_port(third_port, buffer);
				printf("buffer: %s\n", buffer);
			}
			else
			{
				break;
			}	
		}
		else
		{
			knock_port(first_port, buffer);
			knock_port(second_port, buffer);
			knock_port(third_port,buffer);
		}
	
	}
}