#include "client.h"

Client::Client(int f, std::string n)
{
    fd = f;
    name = n;
};

void Client::set_fd(int f)
{
    fd = f;
}
void Client::set_name(std::string n)
{
    name = name;
}


int Client::get_fd()
{
    return fd;
}

std::string Client::get_name()
{
    return name;
}

