#ifndef CONSOLE_LOG_H
#define CONSOLE_LOG_H

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>



// ConsoleLogger provides thread-safe logging utilities with severity levels.
//
// This class allows for logging messages to the console with various log levels
// (INFO, WARNING, ERROR, and CRITICAL ERROR). It uses ANSI color codes to format
// the output for better readability. Thread safety is ensured through the use
// of a mutex, and critical errors throw exceptions for robust error handling.
class ConsoleLogger {
	public:
		
		// Enumeration of log severity levels.
		// 
		// The `LogType` enum specifies the severity of log messages:
		// - `C_CRITICAL_ERROR`: Logs a critical error and throws a runtime exception.
		// - `C_ERROR`: Logs an error message in red.
		// - `C_WARNING`: Logs a warning message in yellow.
		// - `C_INFO`: Logs an informational message in green.
		enum class LogType {
			C_CRITICAL_ERROR, //< Critical error that throws an exception using std::runtime_error.
			C_ERROR,          //< Error message (red text).
			C_WARNING,        //< Warning message (yellow text).
			C_INFO            //< Informational message (green text).
		};


		// Logs a message with a specific severity level.
		// 
		// This method supports variadic arguments to allow seamless logging of
		// multiple types of data. Thread-safe. ANSI color codes are used for
		// formatting. Critical errors throw exceptions.
		// 
		// @param LogType logs The severity level of the log message.
		// @param Args Variadic arguments to be logged.
		// 
		// @throws std::runtime_error for `LogType::C_CRITICAL_ERROR`.
		template<typename... Args>
		static void Print(LogType log, const Args&... args) {
			std::ostringstream oss;
			(oss << ... << args); // Fold expression to handle multiple arguments

			// Use block scope to ensure lock_guard releases the mutex before exceptions
			{
				std::lock_guard<std::mutex> lock(consoleMutex);

				switch (log) {
				case LogType::C_CRITICAL_ERROR:
					std::cerr << "\033[31m[CRITICAL ERROR]::\033[0m " << oss.str() << std::endl;
					break;
				case LogType::C_ERROR:
					std::cerr << "\033[31m[ERROR]::\033[0m " << oss.str() << std::endl;
					break;
				case LogType::C_WARNING:
					std::cerr << "\033[33m[WARNING]::\033[0m " << oss.str() << std::endl;
					break;
				case LogType::C_INFO:
				default:
					std::cout << "\033[32m[INFO]::\033[0m " << oss.str() << std::endl;
					break;
				}
			}

			// Throw the exception *after* releasing the mutex
			if (log == LogType::C_CRITICAL_ERROR) {
				throw std::runtime_error("[CRITICAL ERROR]::" + oss.str());
			}

		}
		
		
		// Logs a message with the default severity level (INFO).
		// 
		// This is an overload of `Print` where the default log level
		// is set to `C_INFO`. Thread-safe.
		// 
		// @tparam Args Variadic template to accept multiple arguments.
		// @param args Variadic arguments to be logged.
		template<typename... Args>
		static void Print(const Args&... args) {
			Print(LogType::C_INFO, args...);
		}

	private:
		// Mutex for thread-safe logging
		static inline std::mutex consoleMutex;
};


#define CONSOLE_LOG_CRITICAL_ERROR(...) \
    ConsoleLogger::Print(ConsoleLogger::LogType::C_CRITICAL_ERROR, "[File: " __FILE__ ", Line: " + std::to_string(__LINE__) + "] ", __VA_ARGS__)

#define CONSOLE_LOG_ERROR(...) \
    ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "[File: " __FILE__ ", Line: " + std::to_string(__LINE__) + "] ", __VA_ARGS__)

#define CONSOLE_LOG_WARNING(...) \
    ConsoleLogger::Print(ConsoleLogger::LogType::C_WARNING, __VA_ARGS__)

#define CONSOLE_LOG_INFO(...) \
    ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, __VA_ARGS__)


#endif // !CONSOLE_LOG_H

