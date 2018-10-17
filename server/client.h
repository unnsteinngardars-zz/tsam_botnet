#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class Client{

    private:
    int fd;
    std::string name;
    public:
    Client(int fd, std::string name);
    int get_fd();
    std::string get_name();
    void set_fd(int f);
    void set_name(std::string n);
};

#endif