#pragma once

#include <iostream>

//#ifndef LOG_STREAM
#define LOG_STREAM(level) \
    std::cout << "logging.h level:" << level << std::endl;
//#endif
