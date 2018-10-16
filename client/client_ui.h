#ifndef _CLIENT_UI_H
#define _CLIENT_UI_H

#include "ncurses.h"
#include "client.h"
#include <string>

class ClientUI
{
    public:
        ClientUI();
        void Start();
        
    private:
        void RenderUI();
        void RenderUI(std::string str);
        int CheckMessages();
        
        void PrintLineAt(int x, int y, const string str);
        void UpdateNames();
        bool StartsWithCaseInsensitive(std::string mainStr, std::string toMatch);
        std::vector<std::string> explode(std::string const & s, char delim);
        std::string GetServerInfo();

        std::string _msg;
        vector<std::string> _messages;
        vector<std::string> _users;
        Client _c;
        WINDOW* _curse_window;
        std::string _paddington;
        int _width, _height;
};

#endif
