#include "memory_resource.h"

namespace folly {
namespace hazptr {

memory_resource** default_mr_ptr() {
  /* library-local */ static memory_resource* default_mr =
      new_delete_resource();
  DEBUG_PRINT(&default_mr << " " << default_mr);
  return &default_mr;
}

memory_resource* get_default_resource() {
  DEBUG_PRINT("");
  return *default_mr_ptr();
}

void set_default_resource(memory_resource* mr) {
  DEBUG_PRINT("");
  *default_mr_ptr() = mr;
}

memory_resource* new_delete_resource() {
  class new_delete : public memory_resource {
   public:
    void* allocate(const size_t bytes, const size_t alignment = max_align_v)
        override {
      (void)alignment;
      void* p = static_cast<void*>(new char[bytes]);
      DEBUG_PRINT(this << " " << p << " " << bytes);
      return p;
    }
    void deallocate(
        void* p,
        const size_t bytes,
        const size_t alignment = max_align_v) override {
      (void)alignment;
      (void)bytes;
      DEBUG_PRINT(p << " " << bytes);
      delete[] static_cast<char*>(p);
    }
  };
  static new_delete mr;
  return &mr;
}

} // namespace hazptr
} // namespace folly
