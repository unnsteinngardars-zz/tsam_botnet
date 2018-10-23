#ifndef SERVER_CONNECTION_H
#define SERVER_CONNECTION_H

#include <string>
#include <ctime>

class ServerConnection{

    private:
    int fd;
    std::string id;
    std::string host;
    int port;
    time_t last_keepalive_sent;
    time_t last_keepalive_recieved;
    public:
    ServerConnection();
    int get_fd();
    std::string get_id();
    std::string get_host();
    int get_port();
    time_t get_time_sent();
    time_t get_time_recieved();


    void update_keep_sent();
    void update_keep_recieved();
    void set_fd(int f);
    void set_id(std::string i);
    void set_host(std::string h);
    void set_port(int p);
};

#endif