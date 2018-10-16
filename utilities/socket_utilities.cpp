#include "socket_utilities.h"

void socket_utilities::error(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

int socket_utilities::create_tcp_socket()
{
	/* Variable declarations */
	int fd;

	/* Create the socket file descriptor */
	fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* check for error */
	if (fd < 0)
	{
		error("Failed creating TCP socket");
	}

	/* Prevent the Address aldready in use message when the socket still hangs around in the kernel after server shutting down */
	int the_integer_called_one = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &the_integer_called_one, sizeof(the_integer_called_one)) < 0)
	{
		error("Failed to prevent \"Address aldready in use\" message");
	}

	return fd;
}

int socket_utilities::create_udp_socket()
{
	int fd;
	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (fd < 0)
	{
		error("Failed creating UDP socket");
	}
	return fd;
}

int socket_utilities::bind_to_address(int fd, struct sockaddr_in& address, int port)
{
	/* memset with zeroes to populate sin_zero with zeroes */
	memset(&address, 0, sizeof(address));
	/* Set address information */
	address.sin_family = AF_INET;
	// Use INADDR_ANY to bind to the local IP address
	address.sin_addr.s_addr = INADDR_ANY;

	address.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		return -1;
	}
	return 1;
}

int socket_utilities::find_available_port(Connection& connection, int min_port, int max_port)
{	
	int i;
	for (i = min_port; i <= max_port; ++i)
	{
		if (bind_to_address(connection.first, connection.second, i) < 0){
			break;
		}
		else {
			return 1;
		}
	}
	return -1;
}



void socket_utilities::listen_on_socket(int fd)
{
	if (listen(fd, 10) < 0)
	{
		error("Failed to listen");
	}
}


void socket_utilities::close_socket(int fd){
	close(fd);
}

int socket_utilities::write_to_client(int fd, std::string message)
{
	int write_bytes = send(fd, message.c_str(), message.size(), 0);
	return write_bytes;
}

int socket_utilities::connect(int fd, sockaddr_in& address)
{
	int n = connect(fd, (struct sockaddr *)& address, sizeof(address));
	return n;
}