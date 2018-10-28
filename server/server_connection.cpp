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

void ServerConnection::set_ip(std::string i)
{
    ip = i;
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

std::string ServerConnection::get_ip()
{
    return ip;
}

int ServerConnection::get_port()
{
    return port;
}

time_t ServerConnection::get_time_sent()
{
    return last_keepalive_sent;
}

void ServerConnection::update_keep_sent()
{
    last_keepalive_sent = time(0);
}

time_t ServerConnection::get_time_recieved()
{
    return last_keepalive_recieved;
}

void ServerConnection::update_keep_recieved()
{
    last_keepalive_recieved = time(0);
}