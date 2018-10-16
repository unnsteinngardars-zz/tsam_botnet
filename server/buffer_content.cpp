#include "buffer_content.h"

/**
 * Creates an empty BufferContent with 
*/
BufferContent::BufferContent()
{
	command = "";
	sub_command = "";
	body = "";
};

/**
 * Set the file descriptor
*/
void BufferContent::set_file_descriptor(int fd)
{
	socket = fd;
}

/**
 * Get the file descriptor
*/
int BufferContent::get_file_descriptor()
{
	return socket;
}



/**
 * Set the command
*/
void BufferContent::set_command(std::string c)
{
	command = c;
}

/**
 * Get the command
*/
std::string BufferContent::get_command()
{
	return command;
}

/**
 * Set the sub command
*/
void BufferContent::set_sub_command(std::string s)
{
	sub_command = s;
}

/**
 * Get the sub command
*/
std::string BufferContent::get_sub_command()
{
	return sub_command;
}

/**
 * Set the body
*/
void BufferContent::set_body(std::string b)
{
	body = b;
}

/**
 * Get the body
*/
std::string BufferContent::get_body()
{
	return body;
}


