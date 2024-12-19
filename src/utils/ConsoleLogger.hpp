#ifndef CONSOLE_LOG_H
#define CONSOLE_LOG_H

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>


/**
* @brief ConsoleLogger provides thread-safe logging utilities with severity levels.
*
* This class allows for logging messages to the console with various log levels
* (INFO, WARNING, ERROR, and CRITICAL ERROR). It uses ANSI color codes to format
* the output for better readability. Thread safety is ensured through the use
* of a mutex, and critical errors throw exceptions for robust error handling.
*/
class ConsoleLogger {
	public:
		/**
		* @brief Enumeration of log severity levels.
		*
		* The `LogType` enum specifies the severity of log messages:
		* - `C_CRITICAL_ERROR`: Logs a critical error and throws a runtime exception.
		* - `C_ERROR`: Logs an error message in red.
		* - `C_WARNING`: Logs a warning message in yellow.
		* - `C_INFO`: Logs an informational message in green.
		*/
		enum class LogType {
			C_CRITICAL_ERROR, //< Critical error that throws an exception.
			C_ERROR,          //< Error message (red text).
			C_WARNING,        //< Warning message (yellow text).
			C_INFO            //< Informational message (green text).
		};

		/**
		* @brief Logs a message with a specific severity level.
		*
		* This method supports variadic arguments to allow seamless logging of
		* multiple types of data. Thread-safe. ANSI color codes are used for
		* formatting. Critical errors throw exceptions.
		*
		* @tparam Args Variadic template to accept multiple arguments.
		* @param log The severity level of the log message.
		* @param args Variadic arguments to be logged.
		*
		* @throws std::runtime_error for `LogType::C_CRITICAL_ERROR`.
		*/
		template<typename... Args>
		static void consolePrint(LogType log, const Args&... args) {
			std::ostringstream oss;
			(oss << ... << args); // Fold expression to handle multiple arguments

			// Use block scope to ensure lock_guard releases the mutex before exceptions
			{
				std::lock_guard<std::mutex> lock(consoleMutex);

				switch (log) {
				case LogType::C_CRITICAL_ERROR:
					// Unlock the mutex before throwing
					break; // Exit the scope of lock_guard
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
		
		/**
		* @brief Logs a message with the default severity level (INFO).
		*
		* This is an overload of `consolePrint` where the default log level
		* is set to `C_INFO`. Thread-safe.
		*
		* @tparam Args Variadic template to accept multiple arguments.
		* @param args Variadic arguments to be logged.
		*/
		template<typename... Args>
		static void consolePrint(const Args&... args) {
			consolePrint(LogType::C_INFO, args...);
		}

	private:
		// Mutex for thread-safe logging
		static std::mutex consoleMutex;
};

// Definition of the static mutex
std::mutex ConsoleLogger::consoleMutex;


#endif // !CONSOLE_LOG_H

