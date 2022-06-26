#include <iostream>

class Node {
};

class MyNode : public Node {
};

void set(const void* ptr) {
}

int main() {
    const MyNode* p = new MyNode;
    set(p);
    return 0;
}
