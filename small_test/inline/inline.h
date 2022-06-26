
// template class
template <typename T>
class Context {
public:
    inline void hello();
    void world();
};

// non-template class
class Foobar {
public:
    void foo();
    void bar();
};

#include "inline.hpp"
