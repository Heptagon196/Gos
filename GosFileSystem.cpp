#include "GosFileSystem.h"
#include <iostream>
#include <queue>

Gos::FileSystem::FileSystem() {
    files.push_back({ -1, 0, 0 });
    structure[0]["."] = 0;
}

Gos::FileSystem Gos::FileSystem::instance;

int Gos::FileSystem::GetFile(Path path, bool init) {
    int cur = 0;
    for (auto& s : path.lexically_relative(".")) {
        if (instance.structure[cur].find(s.string()) == instance.structure[cur].end()) {
            if (!init) {
                std::cerr << "Error: no such file: " << path << std::endl;
                return cur;
            } else {
                instance.files.push_back({ -1, 0, 0, true });
                instance.structure[cur][s.string()] = instance.files.size() - 1;
            }
        }
        cur = instance.structure[cur][s.string()];
    }
    return cur;
}

void Gos::FileSystem::AddDirectory(Path dir) {
    dir = dir.lexically_relative(".");
    std::queue<std::pair<Path, int>> dirs;
    dirs.push({dir, FileSystem::GetFile(dir, true)});
    instance.files[dirs.front().second].packID = -1;
    instance.files[dirs.front().second].directory = true;
    while (!dirs.empty()) {
        int cur = dirs.front().second;
        for (auto& file: std::filesystem::directory_iterator(dirs.front().first)) {
            if (file.path().string().length() > 1 && file.path().filename().string().starts_with(".")) {
                continue;
            }
            auto& s = instance.structure[cur];
            std::string name = file.path().filename().string();
            if (s.find(name) == s.end()) {
                instance.files.push_back({ -1, 0, 0 });
                s[name] = instance.files.size() - 1;
            }
            int nxt = s[name];
            instance.files[nxt].packID = -1;
            if (std::filesystem::is_directory(file.status())) {
                instance.files[nxt].directory = true;
                dirs.push({ file.path(), nxt });
            } else {
                instance.files[nxt].directory = false;
            }
        }
        dirs.pop();
    }
}

void Gos::FileSystem::AddFile(Path path) {
    File& file = instance.files[FileSystem::GetFile(path, true)];
    file.packID = -1;
    file.directory = false;
}

void Gos::FileSystem::PackDirectory(Path dir, Path saveTo) {
    std::ofstream fout;
    std::vector<Path> files;
    FileSystem::Iterate(dir, std::function([&files](Path path) {
        files.push_back(path);
    }));
    fout.open(saveTo);
    fout << files.size() << std::endl;
    for (int i = 0; i < files.size(); i++) {
        fout << files[i].string() << std::endl;
        fout << std::filesystem::file_size(files[i]) << std::endl;
    }
    for (int i = 0; i < files.size(); i++) {
        std::ifstream fin;
        fin.open(files[i], std::ios::binary);
        fout << fin.rdbuf();
        fin.close();
    }
    fout.close();
}

void Gos::FileSystem::AddPack(Path packPos) {
    std::ifstream fin;
    fin.open(packPos);
    int cnt;
    std::string tmp;
    fin >> cnt;
    getline(fin, tmp);
    std::vector<std::string> path;
    std::vector<int> size;
    for (int i = 0; i < cnt; i++) {
        std::string line;
        int s;
        getline(fin, line);
        path.push_back(line);
        fin >> s;
        getline(fin, tmp);
        size.push_back(s);
    }
    instance.dataPacks.push_back(packPos.string());
    int start = fin.tellg();
    int packID = instance.dataPacks.size() - 1;
    for (int i = 0; i < cnt; i++) {
        instance.files[GetFile(path[i], true)] = { packID, start, size[i], false };
        start += size[i];
    }
    fin.close();
}

void Gos::FileSystem::Iterate(Path dir, std::function<void(Path)> action) {
    dir = dir.lexically_relative(".");
    std::queue<std::pair<Path, int>> dirs;
    dirs.push({dir, FileSystem::GetFile(dir, false)});
    while (!dirs.empty()) {
        int cur = dirs.front().second;
        for (auto& file: instance.structure[cur]) {
            if (file.first == ".") {
                continue;
            }
            Path curPath = dirs.front().first;
            curPath.append(file.first);
            if (instance.files[file.second].directory) {
                dirs.push({curPath, file.second});
            } else {
                action(curPath.lexically_relative("."));
            }
        }
        dirs.pop();
    }
}

std::stringstream Gos::FileSystem::Read(Path path) {
    std::stringstream ss;
    File& file = instance.files[FileSystem::GetFile(path, false)];
    if (file.packID == -1) {
        std::ifstream fin;
        fin.open(path, std::ios::binary);
        ss << fin.rdbuf();
        fin.close();
    } else {
        std::ifstream fin;
        char* buffer = new char[file.fileSize];
        fin.open(instance.dataPacks[file.packID]);
        fin.seekg(file.packPos);
        fin.read(buffer, file.fileSize);
        fin.close();
        ss.write(buffer, file.fileSize);
        delete[] buffer;
    }
    return ss;
}
