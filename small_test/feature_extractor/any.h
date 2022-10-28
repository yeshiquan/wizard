#pragma once

#include <string>
#include <memory>
#include <string_view>
#include <iostream>
#include <base/logging.h>

namespace gflow {

template<typename T>
static constexpr inline const char *helper1() {
    // Must have the same signature as helper2().
    return __PRETTY_FUNCTION__ + __builtin_strlen(__FUNCTION__);
}

template<typename T>
static constexpr inline const char *helper2() {
    size_t prefix_len = __builtin_strlen(helper1<void>()) + __builtin_strlen(__FUNCTION__) - 5;
    return __PRETTY_FUNCTION__ + prefix_len;
}

template<typename T>
static constexpr inline std::string_view type_name() {
    const char *s = helper2<T>();
    return { s, __builtin_strlen(s) - 1 };
}

template<class T>
class StaticTypeId {
public:
    static const ::std::string TYPE_NAME;
    static const std::string_view name() {
        return type_name<T>();
    }
};

template<class T>
const ::std::string StaticTypeId<T>::TYPE_NAME = std::string(StaticTypeId<T>::name());

class Any final {
public:
    enum class Type {
        INSTANCE = 0,
        INT64,
        INT32,
        UINT64,
        UINT32,
        BOOLEAN,
        INT16,
        UINT16,
        DOUBLE,
        FLOAT
    };

    Any() :
        _empty(true),
        _type(Type::INSTANCE),
        _instanse_type(&StaticTypeId<::std::nullptr_t>::TYPE_NAME),
        _pointer(nullptr),
        _const_ref(true) {}

    Any(const Any& other) :
        _empty(other._empty),
        _type(other._type),
        _instanse_type(other._instanse_type),
        _holder(other._holder ? other._holder->clone() : nullptr),
        _pointer(_holder ? _holder->get() : 
            (_type == Type::INSTANCE ? other._pointer :
             reinterpret_cast<const void*>(&_primitive_value))),
        _primitive_value(other._primitive_value),
        _const_ref(other._const_ref) {
        }

    // 单独声明实现非常量引用拷贝，避免被Any<T>(T&&)匹配
    Any(Any& other) : Any(const_cast<const Any&>(other)) {}

    template<typename T>
    Any(T&& value) :
        _empty(false),
        _type(Type::INSTANCE),
        _instanse_type(&StaticTypeId<typename ::std::decay_t<T>>::TYPE_NAME),
        _holder(new InstanseHolter<typename ::std::decay_t<T>>(::std::forward<T>(value))),
        _pointer(_holder->get()),
        _primitive_value(),
        _const_ref(false) {
        }        

    Any(Any&& other) :
        _empty(other._empty),
        _type(other._type),
        _instanse_type(other._instanse_type),
        _holder(::std::move(other._holder)),
        _pointer(_type == Type::INSTANCE ? other._pointer :
             reinterpret_cast<const void*>(&_primitive_value)),
        _primitive_value(other._primitive_value),
        _const_ref(other._const_ref) {
        }

#define GFLOW_SPECIALIZE_FOR_PRIMITIVE(ctype, etype, field) \
    inline explicit Any(ctype value) noexcept : \
        _empty(false), \
        _type(Type::etype), \
        _instanse_type(&StaticTypeId<ctype>::TYPE_NAME), \
        _pointer(&_primitive_value), \
        _const_ref(false) { \
            _primitive_value.field = value; \
        } \
    inline void ref(ctype value) noexcept { \
        _empty = false; \
        _type = Type::etype; \
        _instanse_type = &StaticTypeId<ctype>::TYPE_NAME; \
        _pointer = &_primitive_value; \
        _const_ref = false; \
        _primitive_value.field = value; \
    }
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(bool, BOOLEAN, bool_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(int64_t, INT64, int64_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(int32_t, INT32, int32_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(uint64_t, UINT64, uint64_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(uint32_t, UINT32, uint32_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(int16_t, INT16, int16_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(uint16_t, UINT16, uint16_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(double, DOUBLE, double_v);
    GFLOW_SPECIALIZE_FOR_PRIMITIVE(float, FLOAT, float_v);

    #undef GFLOW_SPECIALIZE_FOR_PRIMITIVE

    void swap(Any& rhs) {
        std::swap(_empty, rhs._empty);
        std::swap(_type, rhs._type);
        std::swap(_instanse_type, rhs._instanse_type);
        std::swap(_holder, rhs._holder);
        std::swap(_pointer, rhs._pointer);
        std::swap(_primitive_value, rhs._primitive_value);
        std::swap(_const_ref, rhs._const_ref);
        set_pointer();
        rhs.set_pointer();    
    }

    void set_pointer() {
        if (_holder) {
            _pointer = _holder->get();
        } else if (_type != Type::INSTANCE) {
            _pointer = reinterpret_cast<const void*>(&_primitive_value);
        } else {
            // 通过ref引用其他对象，不需要修改_pointer
        }
    }

    Any& operator=(const Any& other) {
        Any tmp(other);
        swap(tmp);
        return *this;
    }

    Any& operator=(Any&& other) {
        Any tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    template<typename T>
    Any& operator=(T&& value) {
        Any tmp(std::forward<T>(value));
        swap(tmp);
        return *this;
    }

    template<typename T>
    Any& ref(const T& value) {
        _empty = false;
        _type = Type::INSTANCE;
        _instanse_type = &StaticTypeId<T>::TYPE_NAME;
        _holder.reset();
        _pointer = &value;
        _primitive_value = 0;
        _const_ref = false;
        return *this;
    }

    void clear() {
        _empty = true;
        _type = Type::INSTANCE;
        _instanse_type = &StaticTypeId<::std::nullptr_t>::TYPE_NAME;
        _holder.reset();
    }

    template<typename T>
    T* get() {
        if (!_const_ref && _instanse_type == &StaticTypeId<T>::TYPE_NAME) {
            return reinterpret_cast<T*>(const_cast<void*>(_pointer));
        }
        return nullptr;
    }

    template<typename T>
    void require_same_type(std::string data_name) const {
        if (_instanse_type != &StaticTypeId<T>::TYPE_NAME) {
            LOG(ERROR) << "data type not match " << data_name 
                            << " -> [" << StaticTypeId<T>::TYPE_NAME 
                            << "] VS [" << *_instanse_type << "]";
        }
    }

    template<typename T>
    const T* get() const {
        if (_instanse_type == &StaticTypeId<T>::TYPE_NAME) {
            return reinterpret_cast<const T*>(_pointer);
        }
        return nullptr;
    }

    template<typename T>
    T as() const noexcept;

    template<typename T>
    int32_t to(T& value) const;

    operator bool() const {
        return !_empty;
    }

    Type type() const {
        return _type;
    }

    const ::std::string& instance_type() const {
        return *_instanse_type;
    }

private:
    class Holder {
    public:
        virtual ~Holder() {};
        virtual void* get() = 0;
        virtual Holder* clone() const = 0;
    };

    template<typename T>
    class InstanseHolter : public Holder {
    public:
        InstanseHolter(const T& instance) : _instanse(instance) { 
            //std::cout << "InstanseHolter copy type:" << StaticTypeId<T>::name() << std::endl;
        }
        InstanseHolter(T&& instance) : _instanse(::std::move(instance)) {
            //std::cout << "InstanseHolter move type:" << StaticTypeId<T>::name() << std::endl;
        }

        virtual void* get() {
            return &_instanse;
        }

        virtual InstanseHolter* clone() const {
            return new InstanseHolter(_instanse);
        }

    private:
        T _instanse;
    };

    bool _empty;
    Type _type;
    const ::std::string* _instanse_type;
    ::std::shared_ptr<Holder> _holder;
    const void* _pointer = nullptr;
    union PrimitiveValue {
        PrimitiveValue() : int64_v(0) {}
        PrimitiveValue(const PrimitiveValue& other) : int64_v(other.int64_v) {}
        PrimitiveValue& operator=(const PrimitiveValue& other) {
            int64_v = other.int64_v;
            return *this;
        }
        PrimitiveValue& operator=(int64_t v) {
            int64_v = v;
            return *this;
        }
        int64_t int64_v;
        int32_t int32_v;
        uint64_t uint64_v;
        uint32_t uint32_v;
        int16_t int16_v;
        uint16_t uint16_v;
        bool bool_v;
        double double_v;
        float float_v;
    } _primitive_value;
    bool _const_ref;
};

template<typename T>
inline T Any::as() const noexcept {
    switch (_type) {
        case Type::INT64:
            return *reinterpret_cast<const int64_t*>(_pointer);
        case Type::INT32:
            return *reinterpret_cast<const int32_t*>(_pointer);
        case Type::INT16:
            return *reinterpret_cast<const int16_t*>(_pointer);            
        case Type::BOOLEAN:
            return *reinterpret_cast<const bool*>(_pointer);
        case Type::UINT64:
            return *reinterpret_cast<const uint64_t*>(_pointer);
        case Type::UINT32:
            return *reinterpret_cast<const uint32_t*>(_pointer);
        case Type::UINT16:
            return *reinterpret_cast<const uint16_t*>(_pointer);
        case Type::DOUBLE:
            return *reinterpret_cast<const double*>(_pointer);
        case Type::FLOAT:
            return *reinterpret_cast<const float*>(_pointer);
        default:
            return 0;
    }
}

template<>
inline int32_t Any::to<uint64_t>(uint64_t& value) const {
    switch (_type) {
    case Type::INT32: {
            if (_primitive_value.int32_v >= 0) {
                value = static_cast<uint64_t>(_primitive_value.int32_v);
                return 0;
            }
            return -1;
        }
    case Type::INT64: {
            if (_primitive_value.int64_v >= 0) {
                value = static_cast<uint64_t>(_primitive_value.int64_v);
                return 0;
            }
            return -1;
        }
    case Type::UINT32: {
            value = _primitive_value.uint32_v;
            return 0;
        }
    case Type::UINT64: {
            value = _primitive_value.uint64_v;
            return 0;
        }
    default: {
            return -1;
        }
    }
}

}  // namespace
