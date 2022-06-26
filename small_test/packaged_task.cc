#include <iostream>
#include <future>
#include <memory>
#include <thread>

int main() {

    auto sum = [](int a, int b) {
        return a + b;
    };

    // 用于thread，相比普通的函数，packaged_tasks可以获得函数的返回值。
    std::packaged_task<int(int,int)> task(sum);
    std::future<int> future = task.get_future();
    std::thread t(std::move(task), 1, 2);
    std::cout << "sum result -> " << future.get() << std::endl;
    t.join();

    return 0;
}
