#pragma once

#include <string>
#include <memory>

namespace duer {

template<class T>
class StaticTypeId {
public:
    static const ::std::string TYPE_NAME;
    static const char* name() {
        return __PRETTY_FUNCTION__;
    }
};

template<class T>
const ::std::string StaticTypeId<T>::TYPE_NAME = StaticTypeId<T>::name();

class Any final {
public:
    enum class Type {
        INSTANCE = 0,
        INT64,
        INT32,
        UINT64,
        UINT32
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
        _const_ref(other._const_ref) {}

    Any(Any&& other) :
        _empty(other._empty),
        _type(other._type),
        _instanse_type(other._instanse_type),
        _holder(::std::move(other._holder)),
        _pointer(_type == Type::INSTANCE ? other._pointer :
             reinterpret_cast<const void*>(&_primitive_value)),
        _primitive_value(other._primitive_value),
        _const_ref(other._const_ref) {}

    template<typename T>
    Any(T&& value) :
        _empty(false),
        _type(Type::INSTANCE),
        _instanse_type(&StaticTypeId<typename ::std::decay_t<T>>::TYPE_NAME),
        _holder(new InstanseHolter<typename ::std::decay_t<T>>(::std::forward<T>(value))),
        _pointer(_holder->get()),
        _primitive_value(),
        _const_ref(false) {}

    void swap(Any& rhs) {
        std::swap(_empty, rhs._empty);
        std::swap(_type, rhs._type);
        std::swap(_instanse_type, rhs._instanse_type);
        std::swap(_holder, rhs._holder);
        std::swap(_pointer, rhs._pointer);
        std::swap(_primitive_value, rhs._primitive_value);
        std::swap(_const_ref, rhs._const_ref);
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
    const T* get() const {
        if (_instanse_type == &StaticTypeId<T>::TYPE_NAME) {
            return reinterpret_cast<const T*>(_pointer);
        }
        return nullptr;
    }

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
        InstanseHolter(const T& instance) : _instanse(instance) { }
        InstanseHolter(T&& instance) : _instanse(::std::move(instance)) { }

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
    } _primitive_value;
    bool _const_ref;
};

template<>
inline Any& Any::operator=<const int32_t&>(const int32_t& value) {
    _empty = false;
    _type = Type::INT32;
    _instanse_type = &StaticTypeId<int32_t>::TYPE_NAME;
    _holder.reset();
    _pointer = &_primitive_value;
    _primitive_value.int32_v = value;
    _const_ref = false;
    return *this;
}

template<>
inline Any& Any::operator=<int32_t&>(int32_t& value) {
    return operator=<const int32_t&>(value);
}

template<>
inline Any& Any::operator=<int32_t>(int32_t&& value) {
    return operator=<const int32_t&>(value);
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
