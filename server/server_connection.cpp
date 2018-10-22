#include "server_connection.h"

ServerConnection::ServerConnection()
{
};

void ServerConnection::set_fd(int f)
{
    fd = f;
}
void ServerConnection::set_id(std::string i)
{
    id = i;
}

void ServerConnection::set_host(std::string h)
{
    host = h;
}

void ServerConnection::set_port(int p)
{
    port = p;
}

int ServerConnection::get_fd()
{
    return fd;
}

std::string ServerConnection::get_id()
{
    return id;
}

std::string ServerConnection::get_host()
{
    return host;
}

int ServerConnection::get_port()
{
    return port;
}

