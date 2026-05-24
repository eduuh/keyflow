#include "DebugLog.h"

namespace keyflow {

bool gVerboseLogging = false;

std::ofstream& verboseLogStream() {
    static std::ofstream stream("keyflow_debug.log", std::ios::app);
    return stream;
}

void writeVerboseTimestamp(std::ofstream& out) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmBuf{};
    (void)localtime_s(&tmBuf, &t);

    out << std::put_time(&tmBuf, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
        << millis.count() << ' ';
}

} // namespace keyflow
