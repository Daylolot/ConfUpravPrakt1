#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

struct VfsNode {
    std::string name;
    bool directory = true;
    std::string data;
    std::vector<VfsNode> children;
};

class Vfs {
public:
    void Load(const std::string& path);
    void Dump(std::ostream& out) const;
    bool IsLoaded() const { return loaded_; }
    const VfsNode* Find(const std::string& path, std::string& error) const;
    bool ChangeDirectory(const std::string& path, std::string& error);
    std::string Pwd() const;
    bool Copy(const std::string& source, const std::string& destination,
              bool recursive, std::string& error);

private:
    const VfsNode* Resolve(const std::string& path, std::vector<std::string>& names,
                           std::string& error) const;
    VfsNode* MutableAt(const std::vector<std::string>& names);
    VfsNode root_;
    bool loaded_ = false;
    std::vector<std::string> cwd_;
};
