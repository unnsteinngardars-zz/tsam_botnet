#include "server.h"

/**
 * Entry for the server
*/
int main(int argc, char const *argv[])
{	

	if (argc < 4)
	{
		printf("%s <server_connection_port> <client_connection_port> <udp_port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	Server server = Server(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
	server.set_max_buffer(8000);
	server.run();

	// server.set_scan_destination("localhost");
	// server.scan(49152, 65532, false);
	
	return 0;
}
