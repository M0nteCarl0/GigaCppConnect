#include <string>
#include <vector>
#include <memory>
#include <atomic>

#ifndef GIGACPP_TYPES

class Response
{
public:
	int status;
	std::string message;
	std::string data;
};	

template class  Request

#define GIGACPP_TYPES
#endif
