#include <chrono>
#include <thread>
#include <functional>
#include <iostream>
#include <map>

#include "concurrent/thread_pool.h"
#include "concurrent/thread_pool_executor.h"

using duer::vc::ThreadPool;
using duer::vc::ThreadPoolExecutor;
using duer::vc::thread_pool_async;

class TransferAgent {
public:
    TransferAgent() {
        std::cout << "TransferAgent" << std::endl;
    }

    ~TransferAgent() {
        std::cout << "~TransferAgent" << std::endl;
        _stopped = true;
        if (_heart_thread.joinable() == true) {
            _heart_thread.join();
        }
    }

    void work(int x, std::string msg) {
        std::cout << "work -> " << x << " " << msg << std::endl;
    }

    void heartbeat() {
        while (1) {
            std::cout << "heartbeat begin sleep." << std::endl;
            for (int i = 0; i < 40; ++i) {
                if (_stopped) {
                    break;
                }
                std::cout << "heartbeat sleep 500ms..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            if (_stopped) {
                break;
            }
            std::cout << "heartbeat...enqueue" << std::endl;
            auto future = _thread_pool->enqueue(&TransferAgent::work, this, 3, "hello"); 
        }
    }

    void init() {
        _thread_pool = std::make_shared<ThreadPool>(10, 2000000);
        _heart_thread = std::thread(std::bind(&TransferAgent::heartbeat, this));
        //thd.join();
    }
private:
    bool _stopped {false};
    std::shared_ptr<ThreadPool> _thread_pool;
    std::thread _heart_thread;
};

class TransferGroup {
public:
    int start_transfer() {
        auto p = std::make_shared<TransferAgent>();
        p->init();
        _group["name"] = p;
        return 0;
    }
    int stop_transfer() {
        _group.erase("name");
        return 0;
    }
private:
    typedef std::shared_ptr<TransferAgent> TransferPtr;
    std::unordered_map<std::string, TransferPtr> _group;
};

int main() {
    TransferGroup group;
    std::cout << "start_transfer..." << std::endl;
    group.start_transfer();
    std::cout << "start_transfer return " << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    std::cout << "stop_transfer..." << std::endl;
    group.stop_transfer();
    std::cout << "about to exit..." << std::endl;
}
