#define USE_THREAD_LOCAL_CACHE false

#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#include "hazptr_bench.h"

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    std::string log_conf_file = "./conf/log_afile.conf";

    com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                comspace::CronoAppender::tryAppender);

    /*
    com_registappender("BFILE", AsyncFileAppender::get_appender, 
                AsyncFileAppender::try_appender);
    */

    auto logger = logging::ComlogSink::GetInstance();
    if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
        LOG(FATAL) << "load log conf failed";
        return -1;
    }

    return run_bench();
}

