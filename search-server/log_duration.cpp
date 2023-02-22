#include "log_duration.h"

LogDuration::LogDuration()
    : stream_(std::cerr) {
}

LogDuration::LogDuration(std::string message)
    : message_(message), stream_(std::cerr) {
}

LogDuration::LogDuration(std::string message, std::ostream& stream)
    : message_(message), stream_(stream) {
}

LogDuration::~LogDuration() {
    const auto end_time = std::chrono::steady_clock::now();
    const auto dur = end_time - start_time_;
    stream_ << message_ << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << " ms" << std::endl;
}
