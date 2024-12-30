#include "FileSystem.hpp"

#include "ConsoleLogger.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

/// TODO -> add other methods and ConsoleLogger output info xd
std::string FileSystem::getWorkingDirectory() {
	return std::filesystem::current_path().string();
}

void FileSystem::setWorkingDirectory(const std::string& dirPath) {
	std::filesystem::current_path(dirPath);
}
