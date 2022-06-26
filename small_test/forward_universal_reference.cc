// 为了验证通用引用和完美转发，可以减少临时对象的产生
#include <tuple>
#include <iostream>
#include <map>

class Person {
public:
    Person() = delete;
    Person(std::string name_, int age_): 
        name(name_), age(age_) {
        std::cout << "Person construct2()" << std::endl;
    }
    Person(Person const& p) {
        std::cout << "Person Copy construct()" << std::endl;
        name = p.name;
        age = p.age;
    }
    Person& operator=(Person const& p) {
        std::cout << "Person Copy assign()" << std::endl;
        name = p.name;
        age = p.age;
        return *this;
    }
    Person(Person&& p) {
        std::cout << "Person Move construct()" << std::endl;
        name = std::move(p.name);
        age = p.age;
    }
    Person& operator=(Person && p) {
        std::cout << "Person Move assign()" << std::endl;
        name = std::move(p.name);
        age = p.age;
        return *this;
    }

    //Person(Person const&) = delete;             // Copy construct
    //Person(Person&&) = delete;                  // Move construct
    //Person& operator=(Person const&) = delete;  // Copy assign
    //Person& operator=(Person &&) = delete;      // Move assign
public:
    std::string name;
    int age;
};

std::ostream& operator<<(std::ostream& out,const Person& person) {
    out << "{" << person.name << "," << person.age << "}";
    return out;
}

template <
    typename KeyType,
    typename ValueType>
class ValueHolder {
public:
    typedef std::pair<const KeyType, ValueType> value_type;

    explicit ValueHolder(const ValueHolder& other) : item_(other.item_) {}

    // 使用初始化列表，原地构造
    template <typename Arg, typename... Args>
    ValueHolder(std::piecewise_construct_t, Arg&& k, Args&&... args)
      : item_(
            std::piecewise_construct,
            std::forward_as_tuple(std::forward<Arg>(k)),
            std::forward_as_tuple(std::forward<Args>(args)...)) {}

    //ValueHolder(ValueHolder const&) = delete;             // Copy construct
    ValueHolder(ValueHolder &&) = delete;                  // Move construct
    ValueHolder& operator=(ValueHolder const&) = delete;  // Copy assign
    ValueHolder& operator=(ValueHolder &&) = delete;      // Move assign

    value_type& getItem() {
        return item_;
    }

public:
    // 使用std::pair
    value_type item_;
};

template <
    typename KeyType,
    typename ValueType>
class HashMap {
public:
    class NodeT {
    public:
        // 使用初始化列表，原地构造
        template <typename Arg, typename...Args>
        NodeT(Arg&& k, Args&&... args) :
            item_(
                std::piecewise_construct, 
                std::forward<Arg>(k),
                std::forward<Args>(args)...
            ) {
        }
    public:
        ValueHolder<KeyType, ValueType> item_;
    };

    // 使用Universal Reference的参数，既能绑定右值，也能绑定左值
    template <typename Key, typename Value>
    void insert(Key&& k, Value&& v) {
        // 完美转发，确保左值和右值的属性不变
        auto* node = new NodeT(std::forward<Key>(k), std::forward<Value>(v));
        //auto* node = new NodeT(k, v);
    }
};

int main() {
    HashMap<std::string, Person> hs;
    // 传入1个右值
    hs.insert("hello", Person("hello", 34));
    //Person p("jordan", 54);
    //hs.insert("jordan", p);
}
