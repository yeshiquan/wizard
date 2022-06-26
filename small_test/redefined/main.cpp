#include "logging.h"
#include "base_logging.h" // 后面的会覆盖前面的宏定义
#include <iostream>

int main() {
    LOG_STREAM(5);
    return 0;
}
