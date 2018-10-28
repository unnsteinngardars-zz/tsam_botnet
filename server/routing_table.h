#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include "../utilities/time_utilities.h"

using namespace std;

typedef pair<string, string> host_pair_t;
typedef pair<string, vector<string>> table_pair_t;

/**
 * RoutingTable
 * The class that contains all information and handles all logic for the routing table
 * Contains a hash map of server id's as key and a vector of server id's as value.
 * Each vector contains the path to that server
*/
class RoutingTable
{
    private:
    string my_id;
    map<string, vector<string>> table;
    public:
    RoutingTable(string id);
    RoutingTable();
    void set_id(string id);
    void add(string from_id, string added_id);
    void remove(string removed_id);
    void clear();
    string to_string();
    vector<host_pair_t> get_hosts();
};

#endif