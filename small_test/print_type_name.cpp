#include <cstddef>
#include <iostream>
#include <string_view>

template<typename T>
static constexpr inline const char *helper1() {
    // Must have the same signature as helper2().
    return __PRETTY_FUNCTION__ + __builtin_strlen(__FUNCTION__);
}

template<typename T>
static constexpr inline const char *helper2() {
    size_t prefix_len = __builtin_strlen(helper1<void>()) + __builtin_strlen(__FUNCTION__) - 5;
    return __PRETTY_FUNCTION__ + prefix_len;
}

template<typename T>
static constexpr inline std::string_view type_name() {
    const char *s = helper2<T>();
    return { s, __builtin_strlen(s) - 1 };
}

template<typename T>
void print_type_name() {
    std::cout << type_name<T>() << std::endl;
}

struct Context {
    struct Foobar {};
};

int main()
{
    print_type_name<int>();
    print_type_name<size_t>();
    print_type_name<void>();
    print_type_name<std::string_view>();
    print_type_name<Context>();
    print_type_name<Context::Foobar>();
}
