#include <unistd.h>
#include <pwd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

std::string Prompt() { return UserName() + "@" + HostName() + ":~$ "; }

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

Result Run(const std::string& line, const Config& config) {
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
    } else if (command == "ls" || command == "cd") {
        if (words.size() > 2) std::cerr << command << ": too many arguments\n";
        else {
            std::cout << command;
            for (size_t i = 1; i < words.size(); ++i) std::cout << " " << words[i];
            std::cout << '\n';
            return Result::Continue;
        }
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
            std::cout << Prompt() << line << '\n' << std::flush;
            Result result = Run(line, config);
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
        std::cout << Prompt() << std::flush;
        if (!std::getline(std::cin, line)) break;
        if (Run(line, config) == Result::Exit) break;
    }
    return script_failed ? 1 : 0;
}
