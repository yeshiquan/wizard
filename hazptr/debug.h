/*
 * Copyright 2017 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"

//#include <glog/logging.h>

#ifndef FOLLY_ALWAYS_INLINE 
#define FOLLY_ALWAYS_INLINE inline
#endif

#ifndef FOLLY_NOINLINE 
#define FOLLY_NOINLINE __attribute__((__noinline__))
#endif

#ifndef LIKELY 
#define LIKELY(x)   (__builtin_expect((x), 1))
#endif

#ifndef UNLIKELY 
#define UNLIKELY(x) (__builtin_expect((x), 0))
#endif

#ifndef FOLLY_STATIC_CTOR_PRIORITY_MAX 
#define FOLLY_STATIC_CTOR_PRIORITY_MAX __attribute__((__init_priority__(102)))
#endif

#ifndef HAZPTR_DEBUG
#define HAZPTR_DEBUG false
#endif

#define HAZPTR_DEBUG_PRINT(...)                      \
  do {                                               \
    if (HAZPTR_DEBUG) {                              \
      VLOG(2) << __func__ << " --- " << __VA_ARGS__; \
    }                                                \
  } while (false)

#define DEBUG_PRINT(...)                      \
  do {                                               \
    if (HAZPTR_DEBUG) {                              \
      VLOG(2) << __func__ << " --- " << __VA_ARGS__; \
    }                                                \
  } while (false)


constexpr auto kIsDebug = true;
constexpr bool kIsArchArm = false;
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;

// thank you brother tj & stephen (https://github.com/clibs/unlikely)
#define likely(expr) (__builtin_expect(!!(expr), 1))
#define unlikely(expr) (__builtin_expect(!!(expr), 0))

// overload if you want your own abort fn - w/e
#ifndef CHECK_ABORT_FUNCTION
// need that prototype def
# include <stdlib.h>
# define CHECK_ABORT_FUNCTION abort
#endif

// check and bail if nope
#ifndef CHECK
# define CHECK(expr) if (!likely(expr)) { CHECK_ABORT_FUNCTION(); }
#endif

#ifdef DCHECK_IS_ON
#  define DCHECK(...) CHECK(__VA_ARGS__);
#else
#  define DCHECK(expr) ((void) expr);
#endif

#ifndef NOTREACHED
#define NOTREACHED() CHECK(0)
#endif

// alias `NOTREACHED()` because YOLO
#ifndef NOTREACHABLE
#define NOTREACHABLE NOTREACHED
#endif


namespace folly {

template <class T>
inline constexpr
typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long)),
  unsigned int>::type
findLastSet(T x) {
    return x ? ((8 * sizeof(unsigned long) - 1) ^ __builtin_clzl(x)) + 1 : 0;
}

template <class T>
inline constexpr
typename std::enable_if<
  std::is_integral<T>::value && std::is_unsigned<T>::value,
  T>::type
nextPowTwo(T v) {
    return v ? (T(1) << findLastSet(v - 1)) : 1;
}

/**
 * Population count
 */
template <class T>
inline typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) <= sizeof(unsigned int)),
  size_t>::type
  popcount(T x) {
  return size_t(__builtin_popcount(x));
}

template <class T>
inline typename std::enable_if<
  (std::is_integral<T>::value &&
   std::is_unsigned<T>::value &&
   sizeof(T) > sizeof(unsigned int) &&
   sizeof(T) <= sizeof(unsigned long long)),
  size_t>::type
  popcount(T x) {
  return size_t(__builtin_popcountll(x));
}

template <class T>
inline constexpr typename std::enable_if<
    std::is_integral<T>::value && std::is_unsigned<T>::value,
    bool>::type
isPowTwo(T v) {
  return (v != 0) && !(v & (v - 1));
}

} // namespace


