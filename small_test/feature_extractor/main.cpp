#include <iostream>
#include <type_traits>
#include <vector>
#include <tuple>

#include "any.h"

// 去除引用得到本体
#define VALUE_REAL_TYPE \
typename ::std::remove_cv<typename ::std::remove_reference<V>::type>::type

// 引用的情况下得到本体，指针则得到nullptr的type用于默认值
#define DEPEND_DEFAULT_TYPE(index) \
typename ::std::remove_cv<typename ::std::conditional< \
			::std::is_lvalue_reference<D##index>::value, \
			typename ::std::remove_reference<D##index>::type, \
			::std::nullptr_t>::type>::type

// 去除引用或指针得到本体
#define DEPEND_REAL_TYPE(index) \
typename ::std::remove_cv<typename ::std::conditional< \
			::std::is_lvalue_reference<D##index>::value, \
			typename ::std::remove_reference<D##index>::type, \
			typename ::std::remove_pointer<D##index>::type>::type>::type

// 得到依赖类型的指针，T& -> T*，T* -> T**，为了后续可以统一通过*转回
#define DEPEND_POINTER_TYPE(index) \
typename ::std::conditional<::std::is_lvalue_reference<D##index>::value, \
	DEPEND_REAL_TYPE(index)*, \
	DEPEND_REAL_TYPE(index)**>::type
	

#define DECLARE_DEPENDENCIE(index) \
	const duer::Any* a##index = dependencies.get(index); \
    std::cout << "index:" << index << " any:" << a##index << std::endl; \
	const DEPEND_REAL_TYPE(index)* prd##index = nullptr; \
	prd##index = a##index->get<DEPEND_REAL_TYPE(index)>(); \
	DEPEND_POINTER_TYPE(index) pd##index = std::is_lvalue_reference<D##index>::value ? \
			(DEPEND_POINTER_TYPE(index))prd##index :	 \
			(DEPEND_POINTER_TYPE(index))&prd##index ;	

class Feature {
public:
	const duer::Any* get() const {
		return &_value;	
	}
	template<typename T>
	void ref(const T& v) {
        _value.ref(v);
	}
	template<typename T>
	void assign(const T& v) {
        _value = v;
    }
private:
	duer::Any _value;
};

class Dependencies {
public:
	const duer::Any* get(int32_t index) const {
		return _features[index]->get();
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

template<typename V, typename ...D>
class SimpleOperatorBase : public OperatorBase<V> {
protected:
    void defaults(D... d) {
        _dependency_defaults = ::std::make_tuple(d...);
    }

    ::std::tuple<D...> _dependency_defaults;
};

template<typename V, typename D0, typename D1>
class SimpleOperator : public SimpleOperatorBase<
	VALUE_REAL_TYPE,
    DEPEND_DEFAULT_TYPE(0), 
    DEPEND_DEFAULT_TYPE(1)> {
public:
	virtual int32_t gen(V& value, const Dependencies& dependencies) const {
		DECLARE_DEPENDENCIE(0);
		DECLARE_DEPENDENCIE(1);
		return generate(value, *pd0, *pd1);
	}
protected:
	virtual int32_t generate(V value, D0 d0, D1 d1) const = 0;
};

class FoobarOperator : public SimpleOperator<std::string&, const int32_t*, const std::string*> {
	int32_t generate(std::string& result, const int32_t* age, const std::string* name) const override {
        std::cout << "age:" << age << " name:" << name << std::endl;
        result = ("age:" + std::to_string(*age) + " name:" + *name);
	return 0;
}
	
};

int main() {
	int32_t age = 32;
	std::string name = "kenby";
	Feature* f1 = new Feature;
	f1->assign(age);
	Feature* f2 = new Feature;
	f2->ref(name);

	Dependencies dependencies;
	dependencies.add_feature(f1);
	dependencies.add_feature(f2);

	std::string result;
	FoobarOperator* op = new FoobarOperator;
	op->gen(result, dependencies);
	std::cout << "result -> " << result << std::endl;

	return 0;
}
