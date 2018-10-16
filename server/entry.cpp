#include "server.h"

/**
 * Entry for the server
*/
int main(int argc, char const *argv[])
{	

	Server server = Server();
	server.set_max_buffer(1024);
	server.run();

	// server.set_scan_destination("localhost");
	// server.scan(49152, 65532, false);
	
	return 0;
}
