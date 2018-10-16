#ifndef _CLIENT_H
#define _CLIENT_H

#include <string>
#include <vector>
#include "../message_protocol.h"

#define DEFAULT_PORT_MIN 31337
#define DEFAULT_PORT_MAX (DEFAULT_PORT_MIN + 100)
#define KNOCK_SEQUENCE {3, 1, 2}

class Client : public MessageProtocol
{
    public:
        Client() {}
        Client(std::string host, int port);
        Client(std::string host, std::vector<int> ports);

        int GetSocket() { return _master_socket; }
        void SetKnockMode(bool k) { _port_knock_mode = k; }
        void SetUsername(std::string name) { _username = name; }
        int StartClient();
    private:
        std::string _username, _host_str;
        int _port, _master_socket;
        bool _port_knock_mode;
        std::vector<int> _ports;
        std::vector<std::string> _users;
};

#endif