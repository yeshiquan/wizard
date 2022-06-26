#include <mutex>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <vector>
#include <random>
#include <iostream>

// 线程安全
int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

template <typename Key, typename Value>
class HashMapWithMutex {
public:
    Value* mut_find(const Key& key) {
        std::lock_guard<std::mutex> guard(_mutex);
        auto itr = _map.find(key);
        if (itr == _map.end()) {
            return nullptr;
        }
        return &itr->second;
    }

    bool mut_find(const Key& key, Value& val) {
        std::lock_guard<std::mutex> guard(_mutex);
        auto itr = _map.find(key);
        if (itr == _map.end()) {
            return false;
        }
        val = itr->second;
        return true;
    }

    size_t mut_erase(const Key& key) {
        std::lock_guard<std::mutex> guard(_mutex);
        return _map.erase(key);
    }

    size_t mut_size() {
        std::lock_guard<std::mutex> guard(_mutex);
        return _map.size();
    }

    typename std::unordered_map<Key, Value>::iterator mut_erase(
            typename std::unordered_map<Key, Value>::const_iterator pos) {
        std::lock_guard<std::mutex> guard(_mutex);
        return _map.erase(pos);
    }

    Value& operator[](const Key& key) {
        std::lock_guard<std::mutex> guard(_mutex);
        return _map[key];
    }

    Value& operator[](Key&& key) {
        std::lock_guard<std::mutex> guard(_mutex);
        return _map[std::move(key)];
    }
    
    template<typename... Args>
    std::pair<typename std::unordered_map<Key, Value>::iterator, bool> mut_emplace(Args&&... args) {
        std::lock_guard<std::mutex> guard(_mutex);
        return _map.emplace(std::forward<Args>(args)...);
    }

    //用于迭代等场景获取后单独上锁
    std::mutex& get_mutex() {
        return _mutex;
    }

    std::unordered_map<Key, Value>& get_raw_map() {
        return _map;
    }

private:
    std::mutex _mutex;
    std::unordered_map<Key, Value> _map;
};

struct Foobar {
    char data[93];
    std::string message;
    Foobar() {}
    Foobar(std::string msg) {
        message = msg;
    }
};

int main() {
    std::vector<std::thread> threads;
    //using HashMap = std::unordered_map<std::string, Foobar>;
    using HashMap = HashMapWithMutex<std::string, Foobar>;
    HashMap hs;
    size_t key_size = 5;
    std::vector<std::string> keys;
    std::vector<Foobar> values;
    for (int i = 0; i < key_size; ++i) {
        keys.emplace_back("key_" + std::to_string(i+1));
        values.emplace_back(Foobar("value_" + std::to_string(i+1)));
    }
    for (int i = 0; i < key_size; ++i) {
        hs.mut_emplace(keys[i],values[i]);
    }
    for (int i = 0; i < key_size; ++i) {
        auto iter = hs.mut_find(keys[i]);
        //ASSERT_TRUE(iter->message == values[i].message);
    }

    // Reader
    for (size_t i = 0; i < 20; ++i) {
        threads.emplace_back([&, i] {
            while (true) {
                int idx = intRand(1, key_size-1);    
                auto key = keys[idx];
                auto* p_value = hs.mut_find(key);
                int sum = 0;
                if (p_value) {
                    Foobar value;
                    value = *p_value;
                }
                /*
                auto iter = hs.find(key);
                Foobar value;
                if (iter != hs.cend()) {
                    value = iter->second;
                }
                */
            }
        });
    }

    // Writer
    for (size_t i = 0; i < 20; ++i) {
        threads.emplace_back([&] {
            while (true) {
                int idx = intRand(1, key_size-1);    
                auto key = keys[idx];
                auto value = values[idx];
                int flag = intRand(1, 2000);    
                if (flag % 2 == 0) {
                    hs.mut_erase(key);
                } else {
                    //hs.emplace(key, value);
                    hs.mut_emplace(key, value);
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    return 0;
}
