#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <string>

class ServerConnection{

    private:
    int fd;
    std::string id;
    std::string host;
    int port;
    public:
    ServerConnection();
    int get_fd();
    std::string get_id();
    std::string get_host();
    int get_port();

    void set_fd(int f);
    void set_id(std::string i);
    void set_host(std::string h);
    void set_port(int p);
};

#endif