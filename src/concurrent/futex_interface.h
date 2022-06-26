#pragma once

#include <atomic>       // std::atomic
#include <stdint.h>     // int*_t
#include <sys/time.h>   // ::timespec
#include <type_traits>  // std::enable_if

namespace rcu {

// 统一操作接口定义，便于统一支持内核态和用户态futex机制
struct FutexInterface {
    // 用户态提供的futex机制，比如bthread::butex，需要显式创建/销毁
    // 内核态机制一般无需创建，基于对齐地址即可
    constexpr static bool need_create() noexcept;
    inline static uint32_t* create();
    inline static void destroy(uint32_t* futex) noexcept;
    // 核心操作函数，抽象提供最基础的wait和wake功能
    inline static int wait(uint32_t* futex, uint32_t val, const struct ::timespec* timeout) noexcept;
    inline static int wake_one(uint32_t* futex) noexcept;
    inline static int wake_all(uint32_t* futex) noexcept;
    // 睡眠
    inline static void usleep(int64_t us) noexcept;
    // 避让
    inline static void yield() noexcept;
};

// 单个Futex同步量的简化封装版本，消除need_create带来的不同
template <typename F, typename E = void>
class Futex {
public:
    inline Futex();
    inline Futex(const Futex& other);
    inline Futex& operator=(const Futex& other);
    inline ~Futex() noexcept;

    inline Futex(uint32_t value);

    inline ::std::atomic<uint32_t>& value() noexcept;
    inline const ::std::atomic<uint32_t>& value() const noexcept;

    inline int wait(uint32_t val, const struct ::timespec* timeout) noexcept;
    inline int wake_one() noexcept;
    inline int wake_all() noexcept;
};

// 不需要创建的futex版本
template <typename F>
class Futex<F, typename std::enable_if<!F::need_create()>::type> {
public:
    inline Futex() = default;
    inline Futex(const Futex& other) = default;
    inline Futex& operator=(const Futex& other) = default;
    inline ~Futex() noexcept = default;

    inline Futex(uint32_t value);

    inline ::std::atomic<uint32_t>& value() noexcept;
    inline const ::std::atomic<uint32_t>& value() const noexcept;

    inline int wait(uint32_t val, const struct ::timespec* timeout) noexcept;
    inline int wake_one() noexcept;
    inline int wake_all() noexcept;

private:
    uint32_t _value;
};

// 需要创建的futex版本
template <typename F>
class Futex<F, typename std::enable_if<F::need_create()>::type> {
public:
    inline Futex();
    inline Futex(const Futex& other);
    inline Futex& operator=(const Futex& other);
    inline ~Futex() noexcept;

    inline Futex(uint32_t value);

    inline ::std::atomic<uint32_t>& value() noexcept;
    inline const ::std::atomic<uint32_t>& value() const noexcept;

    inline int wait(uint32_t val, const struct ::timespec* timeout) noexcept;
    inline int wake_one() noexcept;
    inline int wake_all() noexcept;

private:
    uint32_t* _value;
};

} // namespace

#include "futex_interface.hpp"
