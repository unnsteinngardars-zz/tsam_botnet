#include <iostream>
#include "routing_table.h"

RoutingTable::RoutingTable(string id)
{
    my_id = id;
}

RoutingTable::RoutingTable() {}

void RoutingTable::set_id(string id)
{
    my_id = id;
}

void RoutingTable::add(string from_id, string added_id)
{   
    if (added_id == my_id || from_id == my_id) { return; }
    vector<string> new_vec;
    if (table.find(from_id) != table.end())
    {
        new_vec = table.at(from_id);
        new_vec.push_back(from_id);
    }
    if (table.find(added_id) != table.end() && table.at(added_id).size() < new_vec.size())
    {
        new_vec = table.at(added_id);
    }
    table.insert(table_pair_t(added_id, new_vec));
}

void RoutingTable::remove(string removed_id)
{
    if (table.find(removed_id) != table.end())
    {
        table.erase(removed_id);
        for(auto it = table.begin(); it != table.end(); ++it)
        {
            if (!it->second.empty())
            {
                table.erase(it->second.at(0));
            }
        }
    }
}

void RoutingTable::clear()
{
    table.clear();
}

string RoutingTable::to_string()
{
    string keys;
    for(auto it = table.begin(); it != table.end(); ++it)
    {
        keys += it->first + " ";
    }
    return keys;
}

vector<host_pair_t> RoutingTable::get_hosts()
{
    vector<host_pair_t> vec;
    for(auto it = table.begin(); it != table.end(); ++it)
    {
        string host = it->first;
        string forward = it->second.empty() ? it->first : it->second.at(0);
        vec.push_back(host_pair_t(host, forward));
    }
    return vec;
}