#include "client_ui.h"
#include <stdio.h>
#include <thread>
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
#include <sstream>
#include <utility>
#include <signal.h>
#include "../utilities/string_utilities.h"

using namespace std;

#define BORDER_SIZE 1

#define SCREEN_WIDTH (COLS - 2)
#define SCREEN_HEIGHT (LINES - 2)

#define INPUT_AREA_START_X BORDER_SIZE
#define INPUT_AREA_START_Y (SCREEN_HEIGHT - BORDER_SIZE*2)
#define INPUT_AREA_HEIGHT 1
#define INPUT_AREA_WIDTH (SCREEN_WIDTH - BORDER_SIZE*2)

#define USER_AREA_WIDTH (10 + BORDER_SIZE*2)
#define USER_AREA_HEIGHT (SCREEN_HEIGHT - INPUT_AREA_HEIGHT - BORDER_SIZE * 2)
#define USER_AREA_START_X (SCREEN_WIDTH - USER_AREA_WIDTH - BORDER_SIZE * 2)
#define USER_AREA_START_Y 1

#define MESSAGE_AREA_START_X (BORDER_SIZE)
#define MESSAGE_AREA_START_Y (INPUT_AREA_START_Y - INPUT_AREA_HEIGHT - BORDER_SIZE)
#define MESSAGE_AREA_WIDTH (SCREEN_WIDTH - USER_AREA_WIDTH - BORDER_SIZE * 2)
#define MESSAGE_AREA_HEIGHT (USER_AREA_HEIGHT)

#define PADDINGTON_Y (INPUT_AREA_START_Y - BORDER_SIZE)
#define PADDINGTON_X (BORDER_SIZE)

/*
**************
*        *usr*
*chat    *lst*
**************
*input       *
**************
*/

ClientUI::ClientUI()
{
    //TODO: Make a main menu type thing
    //cout << "client ui constructor" << endl;
    _c = Client("127.0.0.1", 31338);
    //cout << "connected to server" << endl;
}

int ClientUI::CheckMessages()
{
    fd_set readfds;
    int sock = _c.GetSocket(), stdin_sock = fileno(stdin);
    RenderUI();
    while(true)
    {
        FD_ZERO(&readfds);
        //Listen to the client socket
        FD_SET(sock, &readfds);
        //Listen to stdin
        FD_SET(stdin_sock, &readfds);
        int max_sock = (stdin_sock > sock ? stdin_sock : sock);
        select(max_sock + 1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(sock, &readfds))
        {
            auto msg_suc = _c.ReadMessage(sock);
            string msg = get<0>(msg_suc);
            bool success = get<1>(msg_suc);
            if(success)
            {
                UpdateNames(); //just update names regularly
                //sanitize input
                auto msg_sanitizied = string_utilities::split_by_delimeter(msg, "\n");
                for(auto s : msg_sanitizied)
                {
                    if(s.size() > 0)
                        _messages.insert(_messages.begin(), s);
                }
            }
        }
        if(FD_ISSET(stdin_sock, &readfds))
        {
            switch(int c = getch())
            {
                case ERR: break;
                case 10 : //enter
                    if(!_msg.empty())
                    {
                        _c.SendMessage(_c.GetSocket(), _msg);
                        _command_history.insert(_command_history.begin(), _msg);
                        _cmd_index = -1;
                    }
                    if(_msg == "LEAVE")
                    {
                        close(_c.GetSocket());
                        _msg = "";
                        return 1;
                    }
                    _msg = "";
                    break;
                case KEY_UP:
                    //go up a command (into the past)
                    _cmd_index = min(_cmd_index + 1, (int)_command_history.size()-1);
                    _msg = _command_history[_cmd_index];
                break;
                case KEY_DOWN:
                    //go down a command (into the future)
                    _cmd_index = max(_cmd_index - 1, -1);
                    if(_cmd_index == -1)
                        _msg = "";
                    else
                        _msg = _command_history[_cmd_index];
                break;
                default:
                    if(isalpha(c) || c == ' ' || isdigit(c) || ispunct(c))
                    {
                        if(_msg.size() < (size_t)(MESSAGE_AREA_WIDTH - USER_AREA_WIDTH - BORDER_SIZE * 2))
                            _msg += c;
                    }
                    else if(c == KEY_BACKSPACE || c == 127) //127 is backspace?
                    {
                        if(_msg.length() > 0)
                            _msg.pop_back();
                    }
                    break;
            }
        }
        RenderUI();
    }
}

string ClientUI::GetServerInfo(string old_info)
{
    string con_info = old_info;
    
    RenderUI(con_info);
    while(true)
    {
        switch(int c = wgetch(_curse_window))
        {
            case ERR: break;
            case 10 : //enter
                return con_info;
                break;
            default:
                if(isalpha(c) || c == ' ' || isdigit(c) || ispunct(c))
                {
                    con_info += c;
                }
                else if(c == KEY_BACKSPACE || c == 127) //127 is backspace?
                {
                    if(con_info.length() > 0)
                        con_info.pop_back();
                }
                break;
        }
        RenderUI(con_info);
    }
}

bool ClientUI::StartsWithCaseInsensitive(std::string mainStr, std::string toMatch)
{
    // Convert mainStr to lower case
    std::transform(mainStr.begin(), mainStr.end(), mainStr.begin(), ::tolower);
    // Convert toMatch to lower case
    std::transform(toMatch.begin(), toMatch.end(), toMatch.begin(), ::tolower);

    if(mainStr.find(toMatch) == 0)
        return true;
    else
        return false;
}

void ClientUI::UpdateNames()
{
    _users.clear();
    int msock = _c.GetSocket();
    _c.SendMessage(msock, "WHO");
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(msock, &readfds);
    /*
    Now it returns:
    LIST OF USERS:
     user1
     user2
     ...
     userN

    returns: 
    how many users N
    user1
    user2
    ...
    userN
    */
    select(msock+1, &readfds, NULL, NULL, NULL);
    if(FD_ISSET(msock, &readfds))
    {
        //The return value is just one giant string.
        auto users = _c.ReadMessage(msock);
        //\nLIST OF USERS:\n username\n username\n username\n
        if(get<1>(users))
        {
            auto user_vector = string_utilities::split_by_delimeter(get<0>(users), "\n");
            user_vector.erase(user_vector.begin(), user_vector.begin()+2); //erase the first value (LIST OF USERS)
            for(auto a : user_vector)
            {
                if(a.size() == 0) continue;
                auto usrname = a.substr(1, a.size()); //for some reason, the names begin with a space
                _users.push_back(usrname);
            }
        }
        
        /*auto t_num_users = _c.ReadMessage(msock);
        if(get<1>(t_num_users))
        {
            int num_users = atoi(get<0>(t_num_users).c_str());
            for(int i = 0; i < num_users; i++)
            {
                auto t_user = _c.ReadMessage(msock);
                if(get<1>(t_user))
                    _users.push_back(get<0>(t_user));
            }
        }*/
    }
}

//Renders the ui, with a str in the input area
void ClientUI::RenderUI(string str)
{
    werase(_curse_window);      // clears the window
    box(_curse_window, 0 , 0);  // creates a box around the window; 
                                // the border is actually inside the window,
                                // which is why we make room for it

    //don't overwrite the old border; start just above the input area
    PrintLineAt(PADDINGTON_X, PADDINGTON_Y, _paddington);

    size_t len = min(_messages.size(), (size_t)MESSAGE_AREA_HEIGHT - 1);
    for(size_t i = 0; i < len; i++)
    {
        int msg_y = MESSAGE_AREA_START_Y - i;
        PrintLineAt(MESSAGE_AREA_START_X, msg_y, _messages[i]);
    }
    for(int i = 1; i < USER_AREA_HEIGHT; i++)
    {
        PrintLineAt(USER_AREA_START_X - BORDER_SIZE, i, "|"); //Makes the window look a lot nicer
    }
    for(size_t i = 0; i < _users.size(); i++)
    {
        PrintLineAt(USER_AREA_START_X, USER_AREA_START_Y + i, _users[i]);
    }
    PrintLineAt(INPUT_AREA_START_X, INPUT_AREA_START_Y, ">" + str);

    //Actually draw the screen
    wrefresh(_curse_window);
}

void ClientUI::RenderUI()
{
    RenderUI(_msg);
}

void ClientUI::PrintLineAt(int x, int y, const string str)
{
    mvwprintw(_curse_window, y, x, str.c_str());
}

void ClientUI::Start()
{
    //Lot of setup to make ncurses behave
    noecho();   //don't echo the input char
    cbreak();   //don't remember what this was
    keypad(stdscr, TRUE);   //allow special characters
    signal(SIGPIPE, SIG_IGN);   //ignore sigpipe (protocol some times throws a fit)

    //Setup some values for our new window
    _cmd_index = -1;
    _width = COLS - 2;
    _height = LINES - 2;
    int startx = (COLS - _width)   / 2;
    int starty = (LINES - _height) / 2;
    //Create the ncurses window
    _curse_window = newwin(_height, _width, starty, startx);

    //I don't feel like learning how to use panels right now, so this will have to do
    //Separates our views a bit
    _paddington = string(_width - 2, '-');
    
    //Some helpful messages to get you started
    //_messages.push_back("!!!DO NOT USE NAMES !!! USE IP ADDRESSES!!!");
    //_messages.push_back("The ports will be knocked on in the order given");
    //_messages.push_back("in the format \"ipaddress port1 port2 port3\"");
    _messages.push_back("Type HELP to recieve a list of supported commands");
    _messages.push_back("Once connected, type \"CONNECT,<name>\" to register on the server.");
    _messages.push_back("Please input server details in the form \"ipaddress port\"");
    _messages.push_back("Welcome to the best chat client ever");

    string connection_info;
    vector<string> exploded;
    int leave = 1;
    do
    {
        //Poll the user for some server info
        connection_info = GetServerInfo(connection_info);
        
        std::transform(connection_info.begin(), connection_info.end(), connection_info.begin(), ::tolower);
        if(connection_info == "quit") break;
        //Get our input in a nicer format
        exploded = explode(connection_info, ' ');
        if(exploded.size() < 2)
        {
            _messages.insert(_messages.begin(), "Invalid connection string.");
            _messages.insert(_messages.begin(), "ipaddress port [port2, port3,...,portn]");
            continue;
        }
        string host = exploded[0];
        exploded.erase(exploded.begin());
        
        vector<int> ports;
        bool fail = false;
        for(string i : exploded)
        {
            if(string_utilities::is_number(i))
                ports.push_back(stoi(i));
            else
            {
                _messages.insert(_messages.begin(), "Invalid port " + i + ". Please try again.");
                fail = true;
                break;
            }
        }
        if(fail) continue;
        _messages.insert(_messages.begin(), "Attempting to connect...");
        if(ports.size() == 1)
            _c = Client(host, ports[0]);
        else
            _c = Client(host, ports);

        int connected = _c.StartClient();
        if(connected == 0)
        {
            leave = CheckMessages();
            _messages.insert(_messages.begin(), "Disconnected.");
            _messages.insert(_messages.begin(), "Type quit or press CTRL-C to exit");
        }
        else
        {
            _messages.insert(_messages.begin(), "Failed to connect. Please try again");
        }
    }
    while(leave > 0);
}

std::vector<std::string> ClientUI::explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
    }

    return result;
}