#include "socket_utilities.h"

void socket_utilities::error(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

int socket_utilities::create_socket()
{
	/* Variable declarations */
	int fd;

	/* Create the socket file descriptor */
	fd = socket(PF_INET, SOCK_STREAM, 0);

	/* check for error */
	if (fd < 0)
	{
		error("Failed creating socket");
	}

	/* Prevent the Address aldready in use message when the socket still hangs around in the kernel after server shutting down */
	int the_integer_called_one = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &the_integer_called_one, sizeof(the_integer_called_one)) < 0)
	{
		error("Failed to prevent \"Address aldready in use\" message");
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

int socket_utilities::find_consecutive_ports(int min_port, int max_port, Servers &servers)
{
	for (int i = min_port; i < max_port; ++i)
	{
		for (int j = 0; j < servers.size(); ++j)
		{
			int fd = servers[j].first;
			if (bind_to_address(fd, servers[j].second, i + j) < 0)
			{
				break;
			}
			if (j == servers.size() - 1)
			{
				return 1;
			}
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