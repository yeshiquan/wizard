#include <iostream>

class CompanyA {
public:
    void sendCleartext(const std::string& msg) {
    }
    void sedEncrypted(const std::string& msg) {
    }
};

class CompanyB {
public:
    void sendCleartext(const std::string& msg) {
    }
    void sedEncrypted(const std::string& msg) {
    }
};

class CompanyZ {
public:
    void sedEncrypted(const std::string& msg) {
    }
};


class MsgInfo {};

// 基类，有可能被特化，特化的版本不保证sendClear还存在
template<typename Company>
class MsgSender {
public:
    void sendClear(const MsgInfo& info) {
        std::string msg;
        Company c;
        c.sendCleartext(msg);
    }
};

// 基类，被特化了，特化后没有sendClear函数
template<>
class MsgSender<CompanyZ> {
public:
    void sendSecret(const MsgInfo& info) {
        std::string msg;
        CompanyZ c;
        c.sedEncrypted(msg);
    }
};


template<typename Company>
class LoggingMsgSender : public MsgSender<Company> {
public:
    void sendClearMsg(const MsgInfo& info) {
        std::cout << "log before" << std::endl;
        sendClear(info); // 如果Company == CompanyZ 这个函数不存在，所以编译器不允许
        std::cout << "log after" << std::endl;
    }
};

int main() {
    LoggingMsgSender<CompanyA> sender;
    MsgInfo msg;
    sender.sendClearMsg(msg);
    return 0;
}
