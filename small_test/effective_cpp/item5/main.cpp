#include "named_object.h"
#include <iostream>

int main() {
    std::string newDog = "Persephone";
    std::string oldDog = "Satch";

    NamedObject<int> p(newDog, 2);
    NamedObject<int> s(oldDog, 36);

    p = s;

    std::cout << "done" << std::endl;
    return 0;
}
