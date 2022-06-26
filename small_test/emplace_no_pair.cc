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
    explicit ValueHolder(const ValueHolder& other) : key(other.key), value(other.value) {}

    // 使用初始化列表，原地构造key和value
    template <typename Arg, typename... Args>
    ValueHolder(Arg&& k, Args&&... args)
      : key(std::forward<Arg>(k)), value(std::forward<Args>(args)...) {
        std::cout << "ValueHolder construct()" << std::endl;
    }

    //ValueHolder(ValueHolder const&) = delete;             // Copy construct
    ValueHolder(ValueHolder &&) = delete;                  // Move construct
    ValueHolder& operator=(ValueHolder const&) = delete;  // Copy assign
    ValueHolder& operator=(ValueHolder &&) = delete;      // Move assign
public:
    // 不使用std::pair
    KeyType key;
    ValueType value;
};

template <
    typename KeyType,
    typename ValueType>
class HashMap {
public:
    class NodeT {
    public:
        // 使用初始化列表，原地构造
        template <typename...Args>
        NodeT(Args&&... args) :
            item_(std::forward<Args>(args)...) {
            std::cout << "NodeT construct()" << std::endl;
        }
    public:
        ValueHolder<KeyType, ValueType> item_;
    };

    template <typename... Args>
    void emplace(Args&&... args) {
        auto* node = new NodeT(std::forward<Args>(args)...);
        std::cout << node->item_.key << " -> " << node->item_.value << std::endl;
    }

};

class TestInitializer {
public:
    // 使用初始化列表的好处：只会调用一次Person的构造函数，相当于原地构造。
    /*
    TestInitializer(const std::string& name, int age) : person(name, age) {
        std::cout << "TestInitializer()" << std::endl;
    }
    */

    template <typename... Args>
    TestInitializer(Args&&... args)
      : person(std::forward<Args>(args)...) {}

    /*
    TestInitializer(const Person& p) : person(p) {
        std::cout << "TestInitializer()" << std::endl;
    }
    */
public:
    Person person;
};

int main() {
    /*
    {
        std::cout << "case1 ======================" << std::endl;
        std::pair<std::string, Person> person(
            std::piecewise_construct, //不加这行编译出错
            std::make_tuple("kobe"), 
            std::make_tuple("kobe", 33)
        );

        std::cout << "pair first -> " << person.first << std::endl;
        std::cout << "pair second -> name:[" << person.second.name << "] age:[" << person.second.age << "]" << std::endl;
    }
    */

    {
        std::cout << "\ncase2 ======================" << std::endl;
        HashMap<std::string, Person> hs;
        hs.emplace("hello", Person("hello", 34));
        hs.emplace("hello", "hello", 34);
    }
    /*
    {
        std::cout << "\ncase3 ======================" << std::endl;
        HashMap<std::string, std::string> hs;
        hs.emplace("hello", "jfkdsa");
    }
    {
        std::cout << "\ncase4 ======================" << std::endl;
        TestInitializer ti("kenby", 34);
        std::cout << "name:[" << ti.person.name << "] age:[" << ti.person.age << "]" << std::endl;

        Person p("world", 44);
        TestInitializer ti2(std::move(p));
        std::cout << "name:[" << ti2.person.name << "] age:[" << ti2.person.age << "]" << std::endl;
    }
    */
}
