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
        int* p = nullptr;
        *p = 55;
    }
};

template<typename Company>
class LoggingMsgSender : public MsgSender<Company> {
public:
    // 方法2：使用using，告诉编译器，请它假设
    // sendClear位于base class内
    //using MsgSender<Company>::sendClear;
    void sendClearMsg(const MsgInfo& info) {
        std::cout << "log before" << std::endl;
        // 方法1：假设sendClear将被继承
        //this->sendClear(info); 

        //方法3：明白指出被调用的函数位于base class内
        MsgSender<Company>::sendClear(info);
        std::cout << "log after" << std::endl;
    }
};

int main() {
    LoggingMsgSender<CompanyA> sender;
    MsgInfo msg;
    sender.sendClearMsg(msg);
    return 0;
}
