#ifndef BUFFER_CONTENT_H
#define BUFFER_CONTENT_H

#include <string>

/**
 * BufferContent.h
 * 
 * A class containing the information needed from the recv buffer
*/
class BufferContent
{	
	private:
	
	int socket;
	std::string command;
	std::string sub_command;
	std::string body;
	
	public:
	
	BufferContent();
	
	void set_file_descriptor(int fd);
	int get_file_descriptor();
	
	void set_command(std::string command);
	std::string get_command();
	
	void set_sub_command(std::string sub_command);
	std::string get_sub_command();
	
	void set_body(std::string body);
	std::string get_body();
	
};

#endif