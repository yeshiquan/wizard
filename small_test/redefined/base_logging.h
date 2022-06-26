#pragma once

#include <stdlib.h>
#include <iostream>

class LogStream {};

class LogMessage {
public:
    LogMessage(const char* addr, int32_t line, int32_t level) {
        _stream = new LogStream;
    }
    LogStream& stream() noexcept {
        std::cout << "base_logging.h stream()" << std::endl;
        return *_stream;
    }
private:
    LogStream* _stream = nullptr;
};

//#ifndef LOG_STREAM
#define LOG_STREAM(level) ::LogMessage(__FUNCTION__, __LINE__, level).stream()
//#endif
