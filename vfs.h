#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

// Дерево существует только в памяти. Для файла data хранит его байты.
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

private:
    VfsNode root_;
    bool loaded_ = false;
};
