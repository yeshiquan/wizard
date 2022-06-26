#include <iostream>
#include "inline.h"

int main() {
    Context<int> ctx;
    ctx.hello();
    ctx.world();

    Foobar fb;
    fb.foo();
    fb.bar();
    return 0;
}
