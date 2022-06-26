#include "folly_hazptr.h"

namespace folly {
namespace hazptr {

FOLLY_STATIC_CTOR_PRIORITY_MAX hazptr_domain default_domain_;

hazptr_stats hazptr_stats_;

thread_local hazptr_tls_state tls_state_ = TLS_UNINITIALIZED;
thread_local hazptr_tc tls_tc_data_;
thread_local hazptr_priv tls_priv_data_;
thread_local hazptr_tls_life tls_life_; // last

} // namespace hazptr
} // namespace folly
