
#define GIGACPPCONNECT_VERSION_MAJOR 0
#define GIGACPPCONNECT_VERSION_MINOR 1
#define GIGACPPCONNECT_VERSION_PATCH 0


#include <iostream>
#include <string>
#include <format>


///CRTP class template C++17 for class http::client
template <typename Backend>
class base_http_client {
public:
	std::string Get(const std::string& url) {
		return static_cast<Backend*>(this)->Get_impl(url);
	}

	std::string Post(const std::string& url, const std::string& body) {
		return static_cast<Backend*>(this)->Post_impl(url, body);
	}
	virtual ~base_http_client() = default;
	base_http_client() { std::cout << "Base constructor\n"; }
};

///CRTP class template C++17 for class http::client for mock
class MockClient : public base_http_client<MockClient> {
public:
	MockClient() :_url("test"), _body("test") { std::cout << "Mock constructor\n"; }
	~MockClient() { std::cout << "Mock destructor\n"; _url.clear(); _body.clear(); }
	std::string Get_impl(const std::string& url) {
		std::cout << "Get url: " << url << "\n";
		_url = url;
		_result = std::format("GET Target url {}\n", url);
		return _result;
	}

	std::string Post_impl(const std::string& url, const std::string& body) {
		_url = url;
		_body = body;
		_result = std::format("POST Target url {}\n bodu: {}\n", url, body);
		return _result;
	}

	std::string _url;
	std::string _body;
	std::string _result;

};


template <typename T> std::string Post_test(base_http_client<T>& client) {
	return client.Post("test", "test");
};

template <typename T> std::string Get_test(base_http_client<T>& client) {
	return client.Get("test");
};









