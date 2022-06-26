#include <iostream>
#include "concurrentqueue.h"

int main() {
    moodycamel::ConcurrentQueue<int> q;
    q.enqueue(25);

    int item;
    bool found = q.try_dequeue(item);
    std::cout << "item -> " << item << std::endl;
    assert(found && item == 25);
}
