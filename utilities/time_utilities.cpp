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

