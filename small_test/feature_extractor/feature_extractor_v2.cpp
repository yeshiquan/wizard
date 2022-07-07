#include <iostream>
#include <type_traits>
#include <vector>
#include <tuple>

#include "any.h"


#define REAL_TYPE(T) \
typename ::std::remove_cv_t<typename ::std::conditional_t< \
            ::std::is_lvalue_reference_v<T>, \
            typename ::std::remove_reference_t<T>, \
            typename ::std::remove_pointer_t<T>>>

#define POINTER_TYPE(T) \
typename ::std::conditional_t< \
            ::std::is_lvalue_reference_v<T>, \
			const REAL_TYPE(T)*, \
			const REAL_TYPE(T)**>

#define DECLARE_DEPENDENCIE(i) \
    const duer::Any* a##i = dependencies.get(i); \
    using RealType##i = REAL_TYPE(D##i); \
    using PointerType##i = POINTER_TYPE(D##i);\
    PointerType##i p##i = nullptr; \
	static RealType##i default_value_##i; \
    const RealType##i* tmp##i = a##i->get<RealType##i>(); \
    if (tmp##i == nullptr) { \
        printf("get depend %d value of type##%s## as ##%s## failed\n", \
            i, \
            a##i->instance_type().c_str(), \
            duer::StaticTypeId<RealType##i>::TYPE_NAME.c_str()); \
		tmp##i = &default_value_##i; \
    } \
    if constexpr (std::is_lvalue_reference_v<D##i>) { \
        p##i = tmp##i;\
    } else { \
        p##i = &tmp##i; \
    }

class Feature {
public:
    const duer::Any* get() const {
        return &_value;    
    }
    template<typename T>
    void assign(T&& v) {
        _value = std::forward<T>(v);
    }
private:
    duer::Any _value;
};

class Dependencies {
public:
    const duer::Any* get(int32_t i) const {
        return _features[i]->get();
    }
    void add_feature(Feature* feature) {
        _features.emplace_back(feature);
    }
private:
    std::vector<Feature*> _features;
};


template<typename V, typename ...D>
class OperatorBase {
};

template<typename V, typename D0, typename D1>
class SimpleOperator : public OperatorBase<REAL_TYPE(V), REAL_TYPE(D0), REAL_TYPE(D1)> {
public:
    SimpleOperator() = default;
    virtual int32_t gen(V& value, const Dependencies& dependencies) const {
        DECLARE_DEPENDENCIE(0);
        DECLARE_DEPENDENCIE(1);
        return generate(value, *p0, *p1);
        return 0;
    }
protected:
    virtual int32_t generate(V value, D0 d0, D1 d1) const = 0;
};

// 参数使用指针
class FooOperator : public SimpleOperator<std::string&, const int32_t*, const std::string*> {
    int32_t generate(std::string& result, const int32_t* age, const std::string* name) const override {
        result = ("FooOperator age:" + std::to_string(*age) + " name:[" + *name + "]");
        return 0;
    }
};

// 参数使用引用
class BarOperator : public SimpleOperator<std::string&, const int32_t&, const std::string&> {
    int32_t generate(std::string& result, const int32_t& age, const std::string& name) const override {
        result = ("BarOperator age:" + std::to_string(age) + ";name:[" + name + "]");
        return 0;
    }
};

int main() {
    int32_t age = 32;
    std::string name = "kenby";
    Feature* f1 = new Feature;
    //f1->assign(age);
    Feature* f2 = new Feature;
    //f2->assign(std::move(name));

    Dependencies dependencies;
    dependencies.add_feature(f1);
    dependencies.add_feature(f2);

    std::string result;

    FooOperator* op = new FooOperator;
    op->gen(result, dependencies);
    std::cout << "result -> " << result << std::endl << std::endl;

    BarOperator* op2 = new BarOperator;
    op2->gen(result, dependencies);
    std::cout << "result -> " << result << std::endl;

    return 0;
}
