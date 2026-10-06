#include <unistd.h>
#include <pwd.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "vfs.h"

struct Config {
    std::string vfs_path;
    std::string script_path;
};

void DumpConfig(const Config& config) {
    std::cout << "vfs_path=" << config.vfs_path << '\n';
    std::cout << "script_path=" << config.script_path << '\n';
}

enum class Result { Continue, Exit, Error };

std::string UserName() {
    const passwd* user = getpwuid(getuid());
    return user && user->pw_name ? user->pw_name : "user";
}

std::string HostName() {
    char name[256]{};
    return gethostname(name, sizeof(name)) == 0 ? name : "host";
}

std::string Prompt(const Vfs& vfs) {
    const std::string path = vfs.IsLoaded() && vfs.Pwd() != "/" ? "~" + vfs.Pwd() : "~";
    return UserName() + "@" + HostName() + ":" + path + "$ ";
}

std::vector<std::string> Split(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) words.push_back(word);
    return words;
}

bool ParseOptions(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; i += 2) {
        if (i + 1 >= argc || argv[i + 1][0] == '-') return false;
        const std::string option = argv[i];
        if (option == "--vfs" && config.vfs_path.empty()) config.vfs_path = argv[i + 1];
        else if (option == "--script" && config.script_path.empty()) config.script_path = argv[i + 1];
        else return false;
    }
    return true;
}

Result RunUniq(const std::vector<std::string>& words, const Vfs& vfs) {
    if (words.size() == 1 || (words.size() == 2 && words[1][0] == '-')) {
        std::cerr << "uniq: missing file operand\n";
        return Result::Error;
    }
    if (words.size() > 3) {
        std::cerr << "uniq: too many arguments\n";
        return Result::Error;
    }
    std::string option;
    std::string path;
    if (words.size() == 2) path = words[1];
    else { option = words[1]; path = words[2]; }
    if (!option.empty() && option != "-c" && option != "-d" && option != "-u") {
        std::cerr << "uniq: unsupported option: " << option << '\n';
        return Result::Error;
    }
    std::string error;
    const VfsNode* file = vfs.Find(path, error);
    if (!file) std::cerr << "uniq: " << path << ": " << error << '\n';
    else if (file->directory) std::cerr << "uniq: " << path << ": is a directory\n";
    else if (file->data.find('\0') != std::string::npos)
        std::cerr << "uniq: " << path << ": binary file\n";
    else {
        std::istringstream input(file->data);
        std::string line, previous;
        int count = 0;
        auto print_group = [&] {
            if (count == 0 || (option == "-d" && count == 1) ||
                (option == "-u" && count != 1)) return;
            if (option == "-c") std::cout << std::setw(7) << count << ' ';
            std::cout << previous << '\n';
        };
        while (std::getline(input, line)) {
            if (count != 0 && line != previous) {
                print_group();
                count = 0;
            }
            previous = line;
            ++count;
        }
        print_group();
        return Result::Continue;
    }
    return Result::Error;
}

Result Run(const std::string& line, const Config& config, Vfs& vfs) {
    const auto words = Split(line);
    if (words.empty()) return Result::Continue;
    const std::string& command = words[0];
    if (command == "exit") {
        if (words.size() != 1) std::cerr << "exit: too many arguments\n";
        else return Result::Exit;
    } else if (command == "conf-dump") {
        if (words.size() != 1) std::cerr << "conf-dump: too many arguments\n";
        else {
            DumpConfig(config);
            return Result::Continue;
        }
    } else if (command == "vfs-dump") {
        if (words.size() != 1) std::cerr << "vfs-dump: too many arguments\n";
        else if (!vfs.IsLoaded()) std::cerr << "vfs-dump: no VFS loaded\n";
        else {
            vfs.Dump(std::cout);
            return Result::Continue;
        }
    } else if (command == "pwd") {
        if (words.size() != 1) std::cerr << "pwd: too many arguments\n";
        else if (!vfs.IsLoaded()) std::cerr << "pwd: no VFS loaded\n";
        else {
            std::cout << vfs.Pwd() << '\n';
            return Result::Continue;
        }
    } else if (command == "ls" || command == "cd") {
        if (words.size() > 2) std::cerr << command << ": too many arguments\n";
        else {
            const std::string path = words.size() == 2 ? words[1] :
                                     (command == "cd" ? "/" : ".");
            std::string error;
            if (command == "cd") {
                if (vfs.ChangeDirectory(path, error)) return Result::Continue;
            } else {
                const VfsNode* node = vfs.Find(path, error);
                if (node) {
                    if (node->directory) {
                        for (const auto& child : node->children) std::cout << child.name << '\n';
                    } else std::cout << node->name << '\n';
                    return Result::Continue;
                }
            }
            std::cerr << command << ": " << path << ": " << error << '\n';
        }
    } else if (command == "uniq") {
        return RunUniq(words, vfs);
    } else {
        std::cerr << command << ": command not found\n";
    }
    return Result::Error;
}

int main(int argc, char* argv[]) {
    Config config;
    if (!ParseOptions(argc, argv, config)) {
        std::cerr << "usage: ./emulator [--vfs file.xml] [--script commands.txt]\n";
        return 2;
    }
    DumpConfig(config);
    Vfs vfs;
    if (!config.vfs_path.empty()) {
        try {
            vfs.Load(config.vfs_path);
        } catch (const std::exception& error) {
            std::cerr << "vfs: " << error.what() << '\n';
            return 1;
        }
    }
    bool script_failed = false;
    if (!config.script_path.empty()) {
        std::ifstream script(config.script_path);
        if (!script) {
            std::cerr << "script: cannot open " << config.script_path << '\n';
            return 1;
        }
        std::string line;
        int number = 0;
        while (std::getline(script, line)) {
            ++number;
            if (line.find_first_not_of(" \t\r") == std::string::npos ||
                line.find_first_not_of(" \t\r") == line.find("//")) continue;
            std::cout << Prompt(vfs) << line << '\n' << std::flush;
            Result result = Run(line, config, vfs);
            if (result == Result::Error) {
                std::cerr << "script: " << config.script_path << ':' << number << ": command failed\n";
                script_failed = true;
            }
            if (result == Result::Exit) return script_failed ? 1 : 0;
        }
        if (script.bad()) {
            std::cerr << "script: read error\n";
            return 1;
        }
    }
    std::string line;
    while (true) {
        std::cout << Prompt(vfs) << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (Run(line, config, vfs) == Result::Exit) break;
    }
    return script_failed ? 1 : 0;
}
