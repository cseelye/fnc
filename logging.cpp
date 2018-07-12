#include "logging.h"
#include "spdlog/spdlog.h"


namespace fnc
{

std::shared_ptr<spdlog::logger> setup_logger()
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>());
    auto logger = std::make_shared<spdlog::logger>(fnc_log_name, sinks.begin(), sinks.end());
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] %v");
    logger->set_level(spdlog::level::info);
    return logger;
}

std::shared_ptr<spdlog::logger> LOGGER = setup_logger();

void enable_debug_logging()
{
    get_log()->set_level(spdlog::level::debug);
}

void enable_trace_logging()
{
    get_log()->set_level(spdlog::level::trace);
}

std::shared_ptr<spdlog::logger> get_log()
{
    static std::shared_ptr<spdlog::logger> logger = setup_logger();
    return logger;
}

} // end namespace fnc
