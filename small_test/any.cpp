#include <iostream>
#include <any>
#include <string>
#include <vector>

class Vertex {
public:
    template<typename T>
    const T& get_option() {
        return std::any_cast<const T&>(_option);
    }

    std::any& get_option() {
        return _option;
    }

    template<typename T>
    void set_option(T&& option) {
        _option = std::move(option);
    }
private:
    std::any _option;
};

int main() {
    std::any data;

    std::cout << "has_value() -> " << data.has_value() << std::endl;
    data = std::make_any<std::string>();
    std::cout << "has_value() -> " << data.has_value() << std::endl;

    std::string& v = std::any_cast<std::string&>(data);

    v = "Hello World";

    auto& v2 = std::any_cast<std::string&>(data);
    std::cout << v2 << std::endl;

    std::any data2;
    data2 = std::make_any<std::vector<std::string>>();
    auto& v3 = std::any_cast<std::vector<std::string>&>(data2);

    std::any data3;
    data3 = std::string("HELLO");
    std::cout << std::any_cast<std::string>(data3) << '\n';

    Vertex vertex;
    std::string strategy = "RECALL";
    vertex.set_option(strategy);
    auto& option = vertex.get_option<std::string>();
    std::cout << "option -> " << option << std::endl;

    std::any& any_option = vertex.get_option();
    std::cout << "option -> " << std::any_cast<std::string>(any_option) << std::endl;
    return 0;
}
