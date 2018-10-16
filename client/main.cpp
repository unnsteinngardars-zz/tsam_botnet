#include "client_ui.h"

#define PORT_MIN 31337

using namespace std;

int main(int argc , char *argv[])   
{
    initscr();
    ClientUI c;
    c.Start();
    endwin();
}