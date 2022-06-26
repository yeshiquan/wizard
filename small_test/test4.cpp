#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <mutex>

std::vector<int> queue_data;
std::atomic<int> count;
std::mutex m;
void process(int i)
{

    //std::lock_guard<std::mutex> lock(m);
    std::cout << "thread_id " << std::this_thread::get_id() << " data: " << i << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); //#3
}


void populate_queue()
{
    unsigned const number_of_items = 20;
    queue_data.clear();
    for (unsigned i = 0;i<number_of_items;++i)
    {
        queue_data.push_back(i);
    }

    count.store(number_of_items, std::memory_order_release); //#1 The initial store
}

void consume_queue_items()
{
    while (true)
    {
        int item_index;
        if ((item_index = count.fetch_sub(1, std::memory_order_acquire)) <= 0) //#2 An RMW operation
        {
            std::cout << "thread_id " << std::this_thread::get_id() << " item_index: " << item_index << " -> sleep..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); //#3
            continue;
        }
        process(queue_data[item_index - 1]); //#4 Reading queue_data is safe
    }
}

int main()
{
    std::thread a(populate_queue);
    std::thread b(consume_queue_items);
    std::thread c(consume_queue_items);
    a.join();
    b.join();
    c.join();
}
