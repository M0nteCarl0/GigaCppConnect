#include <string>
#include <vector>
#include <memory>
#include <atomic>

#ifndef GIGACPP_TYPES

class Response
{
public:
	int status{0};
	std::string message{""};
	std::string data{""};
};	

class Request
{
public:
    std::string message{""};
    std::string data{""};
};

#define GIGACPP_TYPES
#endif
