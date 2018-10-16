#include "string_utilities.h"

std::string string_utilities::trim_string(const std::string &s, const std::string &delimiters)
{
	return s.substr(0, s.find_last_not_of(delimiters) + 1);
}

void string_utilities::trim_cstr(char * cstr)
{
	if (cstr != NULL)
	{
		int size = strlen(cstr);
		char temp[size];
		memset(temp, 0, size);
		memcpy(temp, cstr, size + 1);
		std::string str(cstr);
		std::string trimmed_str = trim_string(str);
		memset(cstr, 0, trimmed_str.length());
		memcpy(cstr, trimmed_str.c_str(), trimmed_str.length() + 1);
	}
	return;
}

std::vector<std::string> string_utilities::split_by_delimeter(std::string string_buffer, std::string delimeter)
{
	/* convert to string */
	trim_string(string_buffer);
	size_t pos = 0;
	
	std::string str;
	std::vector<std::string> vec;

	while((pos = string_buffer.find(delimeter)) != std::string::npos)
	{
		str = string_buffer.substr(0, pos);
		vec.push_back(str);
		string_buffer.erase(0, pos + delimeter.length());
	}
	vec.push_back(string_buffer);
	return vec;
}

std::vector<std::string> string_utilities::split_into_commands_and_body(std::string input_string)
{
	trim_string(input_string);
	size_t pos = 0;
	int counter = 0;
	std::string str;
	std::vector<std::string> vec;
	std::string delimeter = " ";
	
	while (((pos = input_string.find(delimeter)) != std::string::npos) && counter < 2)
	{
		str = input_string.substr(0, pos);
		vec.push_back(str);
		input_string.erase(0, pos + delimeter.length());
		counter++;
	}
	vec.push_back(input_string);
	return vec;
}