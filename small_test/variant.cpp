#include <iostream>
#include <variant>

// The class template std::variant represents a type-safe union
// The variant class template is a safe, generic, stack-based discriminated union container, offering a simple solution
// for manipulating an object from a heterogeneous set of types in a uniform manner. Whereas standard containers such as
// std::vector may be thought of as "multi-value, single type," variant is "multi-type, single value."
// std::tuple is "multi-type multi-value"
// std::visit类似boost::apply_visitor

struct {
    void operator()(bool) {
        std::cout << "var is bool!" << std::endl;
    }
    void operator()(double) {
        std::cout << "var is double!" << std::endl;
    }
    void operator()(const std::string&) {
        std::cout << "var is string!" << std::endl;
    }
} visitor;

void test1() {
    std::variant<double, bool, std::string> var;
    var = false;
    std::visit(visitor, var);
    var = 1.2;
    std::visit(visitor, var);
    var = std::string("HELLO");
    std::visit(visitor, var);
}

void test2(){
    std::variant<int, float> v; // v可以表示int或者float
    std::variant<int, float> w; // w可以表示int或者float
   
    v = 42; // v hold int
    int i = std::get<int>(v);

    w = std::get<int>(v);
    w = std::get<0>(v);
    w = v;
}

int main() {
    test1();
    test2();
}
