#include "FileSystem.h"

#include "ConsoleLogger.h"

#include <Windows.h>

#include <ShlObj.h> // For SHGetKnownFolderPath

#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>


/// TODO: add method descriptions
std::string FileSystem::getWorkingDirectory() {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Fetching the current Working Directory.");
	return std::filesystem::current_path().string();
}

void FileSystem::setWorkingDirectory(const std::string& dirPath) {
	std::filesystem::current_path(dirPath);
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Setting the current Working Directory at path: ", getWorkingDirectory());
}

bool FileSystem::createDirectory(const std::string& dirPath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Creating directory: ", dirPath);
	if (std::filesystem::create_directory(dirPath)) {
		return true;
	}
	ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to create directory: ", dirPath);
	return false;
}
bool FileSystem::createDirectories(const std::string& dirPath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Creating directories: ", dirPath);
	if (std::filesystem::create_directories(dirPath)) {
		return true;
	}
	ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to create directories: ", dirPath);
	return false;
}

std::string FileSystem::getAbsolutePath(const std::string& path) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Resolving absolute path: ", path);
	return std::filesystem::absolute(path).string();
}
std::string FileSystem::getRelativePath(const std::string& path, const std::string& basePath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Resolving relative path: ", path, " relative to: ", basePath);
	return std::filesystem::relative(path, basePath).string();
}

bool FileSystem::pathIsEmpty(const std::string& path) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Checking if path is empty: ", path);
	return std::filesystem::is_empty(path);
}
bool FileSystem::pathIsEquivalent(const std::string& sourcePath, const std::string& otherPath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Checking equivalence of paths: ", sourcePath, " and ", otherPath);
	return std::filesystem::equivalent(sourcePath, otherPath);
}
bool FileSystem::pathCopy(const std::string& sourcePath, const std::string& destinationPath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Copying path from: ", sourcePath, " to: ", destinationPath);
	std::error_code ec;
	std::filesystem::copy(sourcePath, destinationPath, std::filesystem::copy_options::overwrite_existing, ec);
	if (ec) {
		ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Copy failed: ", ec.message());
		return false;
	}
	return true;
}
bool FileSystem::pathRemoveAll(const std::string& path) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Removing path: ", path);
	if (std::filesystem::remove_all(path)) {
		return true;
	}
	ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to remove path: ", path);
	return false;
}

bool FileSystem::fileExist(const std::string& filePath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Checking if file exists: ", filePath);
	return std::filesystem::exists(filePath);
}
bool FileSystem::fileRemove(const std::string& filePath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Removing file: ", filePath);
	if (std::filesystem::remove(filePath)) {
		return true;
	}
	ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to remove file: ", filePath);
	return false;
}
bool FileSystem::fileCopy(const std::string& sourcePath, const std::string& destinationPath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Copying file at path: ", sourcePath, " to path: ", destinationPath);
	if (std::filesystem::copy_file(sourcePath, destinationPath)) {
		return true;
	}
	ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to copy file at path: ", sourcePath, " to path: ", destinationPath);
	return false;
}
std::string FileSystem::fileGetName(const std::string& filePath) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Getting file name for: ", filePath);
	return std::filesystem::path(filePath).filename().string();
}

std::string FileSystem::getFileBuffer(const std::string& filePath) {
	std::ifstream file(filePath, std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to open file: ", filePath);
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

std::string FileSystem::getExecutablePath() {
    char buffer[MAX_PATH];
    if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) == 0) {
        ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to retrieve executable path.");
    }
    return std::string(buffer);
}

std::string FileSystem::getUserFolder() {
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
        ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to retrieve the user folder path.");
    }
    std::wstring ws(path);
    CoTaskMemFree(path);
    return std::string(ws.begin(), ws.end());
}

std::string FileSystem::getTempFolder() {
    char buffer[MAX_PATH];
    DWORD result = GetTempPathA(MAX_PATH, buffer);
    if (result == 0 || result > MAX_PATH) {
        ConsoleLogger::Print(ConsoleLogger::LogType::C_ERROR, "Failed to retrieve the temporary folder path.");
    }
    return std::string(buffer);
}

void FileSystem::openFileExplorer(const std::string& path) {
	ConsoleLogger::Print(ConsoleLogger::LogType::C_INFO, "Opening file explorer at: ", path);
	ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
}
std::string FileSystem::toLowerCase(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}
std::string FileSystem::toUpperCase(const std::string& str) {
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}
