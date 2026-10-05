#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string UserName() {
    const char* name = std::getenv("USER");
    return name && *name ? name : "user";
}

std::string HostName() {
    char name[256]{};
    return gethostname(name, sizeof(name)) == 0 ? name : "host";
}

std::vector<std::string> Split(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) words.push_back(word);
    return words;
}

// false means that the REPL should stop.
bool Run(const std::string& line) {
    const auto words = Split(line);
    if (words.empty()) return true;

    const std::string& command = words[0];
    if (command == "exit") {
        if (words.size() != 1) std::cerr << "exit: too many arguments\n";
        else return false;
    } else if (command == "ls" || command == "cd") {
        if (words.size() > 2) {
            std::cerr << command << ": too many arguments\n";
        } else {
            std::cout << command;
            for (size_t i = 1; i < words.size(); ++i) std::cout << " " << words[i];
            std::cout << '\n';
        }
    } else {
        std::cerr << command << ": command not found\n";
    }
    return true;
}

int main() {
    std::string line;
    while (true) {
        std::cout << UserName() << '@' << HostName() << ":~$ " << std::flush;
        if (!std::getline(std::cin, line) || !Run(line)) break;
    }
}
