#include "time_utilities.h"

std::string time_utilities::get_time_stamp()
{
	char buffer[256] = {0};
	time_t raw_time = time(NULL);
	if (raw_time < 0)
	{
		return "";
	}
	struct tm *tmp = localtime(&raw_time); 
	if (tmp == NULL)
	{
		return "";
	}
	strftime(buffer, 256, "%d/%m/%Y-%H:%M:%S", tmp);
	string_utilities::trim_cstr(buffer);
	return std::string(buffer);
}


/**
 * Start the knock timer
*/
void time_utilities::set_timer(time_t& t)
{
	time(&t);
}

/**
 * End the knock timer
*/
void time_utilities::stop_timer(time_t& t)
{
	time(&t);
}


/**
 * Get knock time elapsed in seconds
*/
int time_utilities::get_time_in_seconds(time_t& start, time_t& stop)
{
	return difftime(stop, start);
}