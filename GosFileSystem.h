#pragma once

#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <sstream>

namespace Gos {
    using Path = std::filesystem::path;
    class FileSystem {
        private:
            FileSystem();
            static FileSystem instance;
            struct File {
                int packID;
                int packPos;
                int fileSize;
                bool directory;
            };
            std::vector<std::string> dataPacks;
            std::vector<File> files;
            std::unordered_map<int, std::unordered_map<std::string, int>> structure;
            static int GetFile(Path path, bool init);
        public:
            FileSystem(const FileSystem&) = delete;
            FileSystem(FileSystem&&) = delete;
            FileSystem(FileSystem&) = delete;
            static void AddDirectory(Path dir);
            static void AddFile(Path path);
            static void PackDirectory(Path dir, Path saveTo);
            static void AddPack(Path packPos);

            static void Iterate(Path dir, std::function<void(Path)> action);
            static std::stringstream Read(Path path);
    };
};
