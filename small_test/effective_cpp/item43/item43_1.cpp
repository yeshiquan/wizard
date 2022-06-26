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

class MsgInfo {};

template<typename Company>
class MsgSender {
public:
    void sendClear(const MsgInfo& info) {
        std::string msg;
        Company c;
        c.sendCleartext(msg);
    }
};

template<typename Company>
class LoggingMsgSender : public MsgSender<Company> {
public:
    void sendClearMsg(const MsgInfo& info) {
        std::cout << "log before" << std::endl;
        sendClear(info);
        std::cout << "log after" << std::endl;
    }
};

int main() {
    LoggingMsgSender<CompanyA> sender;
    MsgInfo msg;
    sender.sendClearMsg(msg);
    return 0;
}
