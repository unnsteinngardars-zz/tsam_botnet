#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include <string>
#include <map>
#include <vector>

using namespace std;

typedef pair<string, string> host_pair_t;
typedef pair<string, vector<string>> table_pair_t;


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