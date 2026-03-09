#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <memory>
#include <string_view>

namespace fe::engine {

enum class LogLevel : uint8_t {
  Trace = spdlog::level::trace,
  Debug = spdlog::level::debug,
  Info = spdlog::level::info,
  Warn = spdlog::level::warn,
  Error = spdlog::level::err,
  Fatal = spdlog::level::critical,
  Off = spdlog::level::off
};

class LogManager {
 public:
  static LogManager& Instance();

  void Initialize(std::string_view loggerName = "FANTASY");
  void Shutdown();

  template <typename... Args>
  void Log(LogLevel level, fmt::format_string<Args...> fmt, Args&&... args) {
    if (m_Logger) {
      m_Logger->log(static_cast<spdlog::level::level_enum>(level), fmt, std::forward<Args>(args)...);
    }
  }

  template <typename... Args>
  void Trace(fmt::format_string<Args...> f, Args&&... a) {
    Log(LogLevel::Trace, f, std::forward<Args>(a)...);
  }
  template <typename... Args>
  void Debug(fmt::format_string<Args...> f, Args&&... a) {
    Log(LogLevel::Debug, f, std::forward<Args>(a)...);
  }
  template <typename... Args>
  void Info(fmt::format_string<Args...> f, Args&&... a) {
    Log(LogLevel::Info, f, std::forward<Args>(a)...);
  }
  template <typename... Args>
  void Warn(fmt::format_string<Args...> f, Args&&... a) {
    Log(LogLevel::Warn, f, std::forward<Args>(a)...);
  }
  template <typename... Args>
  void Error(fmt::format_string<Args...> f, Args&&... a) {
    Log(LogLevel::Error, f, std::forward<Args>(a)...);
  }
  template <typename... Args>
  void Fatal(fmt::format_string<Args...> f, Args&&... a) {
    Log(LogLevel::Fatal, f, std::forward<Args>(a)...);
  }

 private:
  LogManager() = default;
  std::shared_ptr<spdlog::logger> m_Logger;
};

#define FE_LOG_TRACE(...) ::fe::engine::LogManager::Instance().Trace(__VA_ARGS__)
#define FE_LOG_DEBUG(...) ::fe::engine::LogManager::Instance().Debug(__VA_ARGS__)
#define FE_LOG_INFO(...) ::fe::engine::LogManager::Instance().Info(__VA_ARGS__)
#define FE_LOG_WARN(...) ::fe::engine::LogManager::Instance().Warn(__VA_ARGS__)
#define FE_LOG_ERROR(...) ::fe::engine::LogManager::Instance().Error(__VA_ARGS__)
#define FE_LOG_FATAL(...) ::fe::engine::LogManager::Instance().Fatal(__VA_ARGS__)

}  // namespace fe::engine