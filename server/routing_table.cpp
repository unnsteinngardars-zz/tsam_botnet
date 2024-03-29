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
    // I do not want to add myself to the table. 
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
        for(auto it = table.begin(); it != table.end(); ++it)
        {
            if (!it->second.empty() && !it->second.at(0).compare(removed_id))
            {

                table.erase(it->first);
            }
        }
        table.erase(removed_id);
    }
}

void RoutingTable::clear()
{
    table.clear();
}

string RoutingTable::to_string()
{
    if (table.empty()) { return my_id + ":"; }
    string str = "";
    for (int i = 0; i < my_id.length(); ++i)
    {
        str += " ";
    }
    str += " ";
    str += my_id + ":";

    for(auto it = table.begin(); it != table.end(); ++it)
    {
        str += it->first + ":";
        for(int i = 1; i < it->second.size(); ++i)
        {
            for(int j = 0; j < it->second.at(i).size(); ++j)
            {
                str += " ";
            }
        }
    }
    str += "\n";
    str += my_id + ":";
    str += "-:";
    for (int i = 0; i < my_id.length() - 1; ++i)
    {
        str += " ";
    }

    for(auto it = table.begin(); it != table.end(); ++it)
    {
        if (it->second.empty())
        {
            str += my_id;
        }
        else
        {
            for(int i = 0; i < it->second.size(); ++i)
            {
                if (i == it->second.size() - 1)
                {
                    str += it->second.at(i);
                }
                else
                {
                    str += it->second.at(i) + "-";
                }
            }
        }
        str += ":";
    }
    return str += "\t" + time_utilities::get_time_stamp();
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