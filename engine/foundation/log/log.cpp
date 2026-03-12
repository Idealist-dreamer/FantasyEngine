#include "log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace fe::engine {

LogManager& LogManager::Instance() {
  static LogManager instance;
  return instance;
}

void LogManager::Initialize(std::string_view loggerName) {
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  consoleSink->set_pattern("%^[%T] [%l] %n: %v%$");

  auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/fantasy.log", 1024 * 1024 * 10, 5);
  fileSink->set_pattern("[%Y-%m-%d %T] [%l] %n: %v");

  std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};

#ifdef _WIN32
  sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif

  m_Logger = std::make_shared<spdlog::logger>(std::string(loggerName), sinks.begin(), sinks.end());
  m_Logger->set_level(spdlog::level::trace);
  m_Logger->flush_on(spdlog::level::err);

  spdlog::register_logger(m_Logger);
}

void LogManager::Shutdown() {
  spdlog::shutdown();
  m_Logger.reset();
}

}  // namespace fe::engine