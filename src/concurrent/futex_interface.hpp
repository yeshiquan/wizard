#pragma once

#include <iostream>

#include "futex_interface.h"

#include <linux/futex.h>    // FUTEX_*
#include <sched.h>          // sched_yield
#include <sys/syscall.h>    // __NR_futex
#include <unistd.h>         // syscall

namespace rcu {

__attribute__((always_inline))
inline constexpr bool FutexInterface::need_create() noexcept {
    return false;
}

__attribute__((always_inline))
inline uint32_t* FutexInterface::create() {
    return new uint32_t;
}

__attribute__((always_inline))
inline void FutexInterface::destroy(uint32_t* futex) noexcept {
    delete futex;
}

__attribute__((always_inline))
inline int FutexInterface::wait(uint32_t* futex, uint32_t val, const struct ::timespec* timeout) noexcept {
    return ::syscall(__NR_futex, futex, (FUTEX_WAIT | FUTEX_PRIVATE_FLAG), val, timeout);
}

__attribute__((always_inline))
inline int FutexInterface::wake_one(uint32_t* futex) noexcept {
    return ::syscall(__NR_futex, futex, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), 1);
}

__attribute__((always_inline))
inline int FutexInterface::wake_all(uint32_t* futex) noexcept {
    return ::syscall(__NR_futex, futex, (FUTEX_WAKE | FUTEX_PRIVATE_FLAG), INT32_MAX);
}

__attribute__((always_inline))
inline void FutexInterface::usleep(int64_t us) noexcept {
    ::usleep(us);
}

__attribute__((always_inline))
inline void FutexInterface::yield() noexcept {
    ::sched_yield();
}

template <typename F>
inline Futex<F, typename std::enable_if<F::need_create()>::type>::Futex() :
    _value(F::create()) {
}

template <typename F>
inline Futex<F, typename std::enable_if<F::need_create()>::type>::Futex(const Futex& other) : Futex() {
    *_value = *other._value;
}

template <typename F>
inline Futex<F, typename std::enable_if<F::need_create()>::type>&
Futex<F, typename std::enable_if<F::need_create()>::type>::operator=(const Futex& other) {
    *_value = *other._value;
    return *this;
}

template <typename F>
inline Futex<F, typename std::enable_if<F::need_create()>::type>::~Futex() noexcept {
    F::destroy(_value);
}

template <typename F>
inline Futex<F, typename std::enable_if<!F::need_create()>::type>::Futex(uint32_t value) : _value(value) {
}

template <typename F>
inline Futex<F, typename std::enable_if<F::need_create()>::type>::Futex(uint32_t value) : Futex() {
    *_value = value;
}

template <typename F>
inline ::std::atomic<uint32_t>&
Futex<F, typename std::enable_if<!F::need_create()>::type>::value() noexcept {
    return reinterpret_cast<::std::atomic<uint32_t>&>(_value);
}

template <typename F>
inline ::std::atomic<uint32_t>&
Futex<F, typename std::enable_if<F::need_create()>::type>::value() noexcept {
    return *reinterpret_cast<::std::atomic<uint32_t>*>(_value);
}

template <typename F>
inline const ::std::atomic<uint32_t>&
Futex<F, typename std::enable_if<!F::need_create()>::type>::value() const noexcept {
    return reinterpret_cast<const ::std::atomic<uint32_t>&>(_value);
}

template <typename F>
inline const ::std::atomic<uint32_t>&
Futex<F, typename std::enable_if<F::need_create()>::type>::value() const noexcept {
    return *reinterpret_cast<const ::std::atomic<uint32_t>*>(_value);
}

template <typename F>
inline int Futex<F, typename std::enable_if<!F::need_create()>::type>::wait(
        uint32_t val, const struct ::timespec* timeout) noexcept {
    return F::wait(&_value, val, timeout);
}

template <typename F>
inline int Futex<F, typename std::enable_if<F::need_create()>::type>::wait(
        uint32_t val, const struct ::timespec* timeout) noexcept {
    return F::wait(_value, val, timeout);
}

template <typename F>
inline int Futex<F, typename std::enable_if<!F::need_create()>::type>::wake_one() noexcept {
    return F::wake_one(&_value);
}

template <typename F>
inline int Futex<F, typename std::enable_if<F::need_create()>::type>::wake_one() noexcept {
    return F::wake_one(_value);
}

template <typename F>
inline int Futex<F, typename std::enable_if<!F::need_create()>::type>::wake_all() noexcept {
    return F::wake_all(&_value);
}

template <typename F>
inline int Futex<F, typename std::enable_if<F::need_create()>::type>::wake_all() noexcept {
    return F::wake_all(_value);
}

} // namespace
