// GigaCppConnect.cpp: определяет точку входа для приложения.
//

#include "GigaCppConnect.h"
#include <memory>
#include <iostream>

int main()
{
	MockClient client;
	std::cout << Get_test(client);
	std::cout << Post_test(client);
	std::cout << "Hello CMake." << "\n";
	return 0;
}
