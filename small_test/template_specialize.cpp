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

class Holder {
public:
    virtual ~Holder() {};
    virtual Holder* clone() const = 0;
};

template<typename T>
class InstanseHolter : public Holder {
public:
    InstanseHolter(const T& instance) : _instanse(instance) {
        std::cout << "1 InstanseHolter const T&" << std::endl;
    }
    InstanseHolter(T&& instance) : _instanse(::std::forward<T>(instance)) {
        std::cout << "1 InstanseHolter T&&" << std::endl;
    }

    virtual InstanseHolter* clone() const {
        return new InstanseHolter(_instanse);
    }

private:
    T _instanse;
};

template<typename T>
class InstanseHolter<T&> : public Holder {
public:
    InstanseHolter(const T& instance) : _instanse(instance) {
        std::cout << "2 InstanseHolter const T&" << std::endl;
    }
    InstanseHolter(T&& instance) : _instanse(::std::forward<T>(instance)) {
        std::cout << "2 InstanseHolter T&&" << std::endl;
    }

    virtual InstanseHolter* clone() const {
        return new InstanseHolter(_instanse);
    }

private:
    T _instanse;
};

template<typename T>
void proxy(T&& value) {
    std::cout << "T = " << type_name<T>() << std::endl;
    Holder* p = new InstanseHolter<T>(::std::forward<T>(value));
    printf("\n");
}

int main() {
    std::string message = "hello";
    const std::string& message2 = message;

    proxy(message);
    proxy(message2);
    proxy(std::move(message));

    return 0;
}
