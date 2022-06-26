#pragma once

#include <string>

template<typename T>
class NamedObject {
public:
    NamedObject(std::string& name, const T& value) : 
            nameValue(name), 
            objectValue(value) 
            {}
private:
    // non-static reference member ‘std::string& NamedObject<int>::nameValue’, can't use default assignment operator
    std::string& nameValue;
    // non-static const member ‘const int NamedObject<int>::objectValue’, can't use default assignment operator
    const T objectValue;

    // 因为引用和const是不可修改的，默认的拷贝构造也无法强行修改，所以会报错
};
