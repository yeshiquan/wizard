#include <iostream>
#include <memory>
#include <vector>

#include <string_view>
#include <type_traits>

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

template<typename T>
void print_type_name(std::string label = "") {
    std::cout << label << " " << type_name<T>() << std::endl;
}

template <class D> struct InnerType;

template<typename T>
class BaseContainer;

template<typename T>
struct NDArray : public BaseContainer<NDArray<T>> {
  using self_type = NDArray<T>;
  using base_type = BaseContainer<self_type>;
  using reference = typename base_type::reference;
  char* get_ptr() { return reinterpret_cast<char*>(dataptr.get()); }
  std::unique_ptr<char[]> dataptr;
};

template<typename T>
struct View : public BaseContainer<View<T>> {
  using self_type = View<T>;
  using base_type = BaseContainer<self_type>;
  using reference = typename base_type::reference;  
  char* get_ptr() { return reinterpret_cast<char*>(ec); }
  T *ec;
};

template <class T>
struct InnerType<View<T>> {
  using reference = T;
};

template <class T>
struct InnerType<NDArray<T>> {
    using reference = T;
};

// D = NDArray<int> 或者 View<int>
template<typename D>
struct BaseContainer {
  using derived_type = D;
  using inner_types = InnerType<D>;
  using reference = typename inner_types::reference;

  // error: invalid use of incomplete type 'struct NDArray<int>'
  // using reference = typename D::reference;

  inline const derived_type& self(void) const {
    return *static_cast<const derived_type*>(this);
  }  
  inline derived_type& self(void) {
    return *static_cast<derived_type*>(this);
  }

  // 这个方法是公共的，可以被子类调用
  void run() {
    std::cout << "run...ptr:" << (void*)self().get_ptr() << std::endl;
    print_type_name<derived_type>("derived_type -> ");
    std::cout << std::endl;
  }
};

int main() {
  NDArray<int> arr;
  arr.run();
  View<int> view;
  view.run();

  return 0;
}
