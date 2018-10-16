#ifndef TIME_UTILITIES_H
#define TIME_UTILITIES_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include "string_utilities.h"

/**
 * Utilities for time functionality
*/
namespace time_utilities
{
	std::string get_time_stamp();
	void set_timer(time_t& t);
	void stop_timer(time_t& t);
	int get_time_in_seconds(time_t& start, time_t& stop);
}

#endif