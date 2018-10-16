#ifndef _MESSAGE_PROTOCOL_H
#define _MESSAGE_PROTOCOL_H

#define HEAD_LEN int32_t

#include <string.h>
#include <iostream>
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>
#include <arpa/inet.h> 
#include <tuple>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


using namespace std;

class MessageProtocol
{
    public:
        int fd_is_valid(int fd)
        {
            errno = 0;
            return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
        }

        int SendMessage(int to, string msg)
        {
            HEAD_LEN len = msg.length();
            if(!fd_is_valid(to)) return -1;

            ssize_t len_fail = send(to, &len, sizeof(HEAD_LEN), 0);
            if(len_fail != sizeof(HEAD_LEN))
                return -1;
            
            if(!fd_is_valid(to)) return -1;
            int ret = send(to, msg.c_str(), len, 0);
            
            return ret;
        }

        char* ReadCount(int socket, int len)
        {
            int bytes_read = 0;
            int result;
            char* buffer = new char[len];
            while(bytes_read < len)
            {
                result = read(socket, buffer + bytes_read, len - bytes_read);
                if(result < 1)
                {
                    delete[] buffer;
                    return nullptr;
                }
                bytes_read += result;
            }
            return buffer;
        }

        tuple<string, bool> ReadMessage(int socket)
        {
            //Our header is 4bytes long, describing the length of the message that follows
            HEAD_LEN msg_len;
            char* c = ReadCount(socket, sizeof(msg_len));
            if(c == nullptr) return make_tuple("", false);
            msg_len = *(HEAD_LEN*)c;
            delete[] c; //remember cleanup

            char* msg = ReadCount(socket, msg_len);
            if(msg == nullptr) return make_tuple("", false);

            string retval = string(msg, msg_len);
            delete[] msg;

            return make_tuple(retval, true);
        }
};

#endif