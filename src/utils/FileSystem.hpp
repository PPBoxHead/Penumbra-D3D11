#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <string>

class FileSystem {
	public:
		static std::string getWorkingDirectory();
		void setWorkingDirectory(const std::string& dirPath);

		static bool createDirectory(const std::string& dirPath);
		static bool createDirectories(const std::string& dirPath);

		static std::string getAbsolutePath(const std::string& path);
		static std::string getRelativePath(const std::string& path, const std::string& basePath = ".");

		static bool pathIsEmpty(const std::string& path);
		static bool pathIsEquivalent(const std::string& sourcePath, const std::string& otherPath);
		static bool pathCopy(const std::string& sourcePath, const std::string& destinationPath);
		static bool pathRemoveAll(const std::string& path);

		static bool fileExist(const std::string& filePath);
		static bool fileRemove(const std::string& filePath);
		static bool fileCopy(const std::string& sourcePath, const std::string& destinationPath);
		static std::string fileGetName(const std::string& filePath);

		static std::string getFileBuffer(const std::string& filePath);
		static const char* getFileBufferC(const std::string& filePath);

		static std::string toLowerCase(const std::string& str);
		static std::string toUpperCase(const std::string& str);
};

#endif // !FILE_SYSTEM_MANAGER_H

