#include "client.h"

#include <stdio.h> 
#include <iostream>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

Client::Client(string host, int port)
{
    _host_str = host;
    _port = port;
}

Client::Client(string host, vector<int> ports)
{
    _host_str = host;
    _ports = ports;
    _port_knock_mode = true;
}

int Client::StartClient()
{
    int retval;
    //Go to the other one!
    if(!_port_knock_mode)
    {
        _master_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(_master_socket < 0)
        {
            //cout << "Error establishing socket" << endl;
            return -1;
        }

        struct sockaddr_in serv_address;
        serv_address.sin_family = AF_INET;
        serv_address.sin_port = htons(_port);

        if(inet_pton(AF_INET, _host_str.c_str(), &serv_address.sin_addr) <= 0)
        {
            //cout << "Invalid address" << endl;
            return -1;
        }
        
        if(connect(_master_socket, (struct sockaddr*)&serv_address, sizeof(serv_address)) < 0)
        {
            //cout << "Connection failed" << endl;
            return -1;
        }
        /*while(1)
        {
            string input = "";
            if(input == "/exit") break;
            getline(cin, input);
            SendMessage(_master_socket, input);
            cout << ReadMessage(_master_socket) << endl;
        }*/
    }
    else
    {
        //So we need to knock on our sequence
        for(int port : _ports)
        {
            //Establish our socket
            _master_socket = socket(AF_INET, SOCK_STREAM, 0);
            if(_master_socket < 0)
            {
                //cout << "Error establishing socket" << endl;
                return -1;
            }

            struct sockaddr_in serv_address;
            serv_address.sin_family = AF_INET;
            serv_address.sin_port = htons(port);
            //Convert our host string to the rquired format
            //Aknowledgement: Does not like names, for example localhost
            if(inet_pton(AF_INET, _host_str.c_str(), &serv_address.sin_addr) <= 0)
            {
                //cout << "Invalid address" << endl;
                return -1;
            }
            retval = connect(_master_socket, (struct sockaddr*)&serv_address, sizeof(serv_address));
            if(retval < 0)
            {
                //cout << "Connection failed to port " << port << endl;
                return -1;
            }
            if(port != _ports[_ports.size() - 1])
                close(_master_socket);
        }
    }
    return retval;
}