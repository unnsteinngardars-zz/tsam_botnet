#include "server.h"

/**
 * Initialize a Server with fortune
*/
Server::Server()
{

	id = "tsamgroup33"; // might later take in argv[0] from main and assign
	FD_ZERO(&active_set);
	struct sockaddr_in s;
	struct sockaddr_in c;
	struct sockaddr_in u;

	server_conn_port.second = s;
	client_conn_port.second = c;
	udp_port.second = u;
	
	server_conn_port.first = socket_utilities::create_tcp_socket();
	client_conn_port.first = socket_utilities::create_tcp_socket();
	udp_port.first = socket_utilities::create_udp_socket();
	// searching for available ports, replace with parameters when implemented fully
	socket_utilities::find_available_port(server_conn_port, 4000, 4100);
	socket_utilities::find_available_port(client_conn_port, 49152, 65535);
	socket_utilities::find_available_port(udp_port, 49152, 65535);
	
	socket_utilities::listen_on_socket(server_conn_port.first);
	socket_utilities::listen_on_socket(client_conn_port.first);

	FD_SET(server_conn_port.first, &active_set);
	FD_SET(client_conn_port.first, &active_set);
	FD_SET(udp_port.first, &active_set);
	max_file_descriptor = server_conn_port.first > client_conn_port.first ? server_conn_port.first : client_conn_port.first;
	max_file_descriptor = udp_port.first > max_file_descriptor ? udp_port.first : max_file_descriptor;
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
	usernames.insert(Client(fd, username));
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
			if (i != server_conn_port.first && i != client_conn_port.first && i != fd)
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
	/* buffer for messages */
	char buffer[MAX_BUFFER_SIZE];
	printf("Listening for client connections on port %d\n", ntohs(client_conn_port.second.sin_port));
	printf("Listening for server connections on port %d\n", ntohs(server_conn_port.second.sin_port));
	printf("Listening for udp connections on port %d\n", ntohs(udp_port.second.sin_port));
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
	
				/* server trying to connect to us */
				/* Add second check later to destinguish between server connecting and client connecting */
				if (i == server_conn_port.first || i == client_conn_port.first)
				{
					client_length = sizeof(client);
					int client_fd = accept_connection(i, client, client_length);
					printf("Connection established from %s port %d and fd %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), client_fd);
					std::string welcome_message = "Welcome, type HELP for available commands\n";
					write_to_client(client_fd, welcome_message);
					FD_SET(client_fd, &active_set);
					update_max_fd(client_fd);
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



