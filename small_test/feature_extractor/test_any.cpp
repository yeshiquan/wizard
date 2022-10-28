#include <gtest/gtest.h>
#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <tuple>
#include <bthread.h>
#include <bthread/mutex.h>


#include "vertex.h"

#define private public
#define protected public
#include "any.h"
#undef private
#undef protected


using namespace gflow;

class AnyTest : public ::testing::Test {
   private:
    virtual void SetUp() {
        // p_obj.reset(new ExprBuilder);
    }
    virtual void TearDown() {
        // if (p_obj != nullptr) p_obj.reset();
    }

   protected:
    // std::shared_ptr<ExprBuilder>  p_obj;
};

struct Context {
    int value = 0;
    Context() {
        std::cout << "Context()\n";
    }
    ~Context() {
        std::cout << "~Context()\n";
    }
    friend std::ostream& operator<<(std::ostream& os, const Context& dt);
};
std::ostream& operator<<(std::ostream& os, const Context& dt) {
    os << "context{value=" << dt.value << "}";
    return os;
}

template<typename T>
void assign(Any& any) {
    any = Any(T());
    std::cout << "any value -> [" << *any.get<T>() << "]" << std::endl;
}

template <typename T>
Any& make() {
    static Any _any_data;
    if (_any_data.get<T>() == nullptr) {
        _any_data = Any(T());
    }
    return _any_data;
}

TEST_F(AnyTest, test_any) {
    Any any;
    std::cout << "pointer -> " << any.get<std::string>() << std::endl;

    std::string msg = "hello world";
    Any any2(msg);
    std::cout << "pointer -> " << any2.get<std::string>() << std::endl;    
    std::cout << "value -> " << *any2.get<std::string>() << std::endl;    

    Any any3;
    assign<int>(any3);
    assign<std::string>(any3);
    assign<Context>(any3); // any负责Context对象的生命周期

    Any any4(false);
    std::cout << "any4 -> " << *any4.get<bool>() << std::endl;
    std::cout << "any4 -> " << any4.as<int>() << std::endl;

    Any any5(true);
    std::cout << "any5 -> " << *any5.get<bool>() << std::endl;
    std::cout << "any5 -> " << any5.as<int>() << std::endl;

    float pi = 3.14;
    Any any6(pi); 
    std::cout << "any6 -> " << *any6.get<float>() << std::endl;   

    int32_t age = 34;
    Any any7(age); 
    std::cout << "any7 -> " << *any7.get<int32_t>() << std::endl;

    Any tmp = Any(age);
    Any any8(std::move(tmp));
    std::cout << "any8 -> " << *any8.get<int32_t>() << std::endl;
    ASSERT_EQ(age, *any8.get<int32_t>());

    Any any9;
    any9 = Any(age);
    std::cout << "any9 -> " << *any9.get<int32_t>() << std::endl;
    ASSERT_EQ(age, *any9.get<int32_t>());    

    Any& mids = make<std::vector<std::string>>();
    auto* vec = mids.get<std::vector<std::string>>();
    vec->clear();
    vec->emplace_back("hello");
}

TEST_F(AnyTest, test_any_case2) {
    Any any1;
    int32_t age = 34;
    any1.ref(age);
    ASSERT_EQ(age, *any1.get<int32_t>());
    std::cout << "any1 -> " << *any1.get<int32_t>() << std::endl << std::endl;    

    Any any2(any1);
    ASSERT_EQ(age, *any2.get<int32_t>());
    std::cout << "any2 -> " << *any2.get<int32_t>() << std::endl;    

    Any any3 = any1;
    ASSERT_EQ(age, *any3.get<int32_t>());
    std::cout << "any3 -> " << *any3.get<int32_t>() << std::endl;        
}

TEST_F(AnyTest, test_any_case3) {
    Any any1;
    Context ctx;
    ctx.value = 999;
    any1.ref(ctx);
    ASSERT_EQ(ctx.value, any1.get<Context>()->value);
    std::cout << "any1 -> " << any1.get<Context>()->value << std::endl;    

    Any any2 = any1;
    ASSERT_EQ(ctx.value, any2.get<Context>()->value);
    std::cout << "any2 -> " << any2.get<Context>()->value << std::endl;
}

TEST_F(AnyTest, test_any_case4) {
    Context ctx;
    ctx.value = 999;
    int32_t age = 34;
    double pi = 3.14;

    Any any1 = ctx;
    Any any2 = age;
    Any any3 = pi;
    ASSERT_EQ(ctx.value, any1.get<Context>()->value);
    ASSERT_EQ(age, *any2.get<int32_t>());
    ASSERT_EQ(pi, *any3.get<double>());

    Any any11 = any1;
    Any any22 = any2;
    Any any33 = any3;
    ASSERT_EQ(ctx.value, any11.get<Context>()->value);
    ASSERT_EQ(age, *any22.get<int32_t>());
    ASSERT_EQ(pi, *any33.get<double>());    
}

TEST_F(AnyTest, test_any_case5) {
    Any any1 = 99.5;  
}
