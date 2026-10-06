#include "vfs.h"

#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

bool Space(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

std::string XmlText(const std::string& value) {
    std::string result;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '&') {
            result += value[i];
            continue;
        }
        const size_t end = value.find(';', i);
        if (end == std::string::npos) throw std::runtime_error("unterminated XML entity");
        const std::string entity = value.substr(i, end - i + 1);
        if (entity == "&amp;") result += '&';
        else if (entity == "&lt;") result += '<';
        else if (entity == "&gt;") result += '>';
        else if (entity == "&quot;") result += '"';
        else if (entity == "&apos;") result += '\'';
        else throw std::runtime_error("unsupported XML entity: " + entity);
        i = end;
    }
    return result;
}

std::string Base64(const std::string& input) {
    const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    for (char c : input) if (!Space(c)) encoded += c;
    if (encoded.size() % 4 != 0) throw std::runtime_error("invalid base64 length");
    std::string output;
    for (size_t i = 0; i < encoded.size(); i += 4) {
        const bool last = i + 4 == encoded.size();
        const bool pad2 = encoded[i + 2] == '=';
        const bool pad3 = encoded[i + 3] == '=';
        if (encoded[i] == '=' || encoded[i + 1] == '=' || (pad2 && !pad3) ||
            (!last && (pad2 || pad3))) throw std::runtime_error("invalid base64 padding");
        const size_t a = alphabet.find(encoded[i]);
        const size_t b = alphabet.find(encoded[i + 1]);
        const size_t c = pad2 ? 0 : alphabet.find(encoded[i + 2]);
        const size_t d = pad3 ? 0 : alphabet.find(encoded[i + 3]);
        if (a == std::string::npos || b == std::string::npos ||
            c == std::string::npos || d == std::string::npos ||
            (pad2 && (b & 15)) || (pad3 && !pad2 && (c & 3))) {
            throw std::runtime_error("invalid base64 data");
        }
        output += static_cast<char>((a << 2) | (b >> 4));
        if (!pad2) output += static_cast<char>(((b & 15) << 4) | (c >> 2));
        if (!pad3) output += static_cast<char>(((c & 3) << 6) | d);
    }
    return output;
}

class XmlParser {
public:
    explicit XmlParser(std::string xml) : xml_(std::move(xml)) {}

    VfsNode Parse() {
        Skip();
        if (Starts("<?xml")) {
            const size_t end = xml_.find("?>", pos_);
            if (end == std::string::npos) Fail("unfinished XML declaration");
            pos_ = end + 2;
        }
        Skip();
        Open("<vfs>");
        VfsNode root;
        ReadChildren(root, "vfs", 0);
        Skip();
        if (pos_ != xml_.size()) Fail("data after </vfs>");
        return root;
    }

private:
    std::string xml_;
    size_t pos_ = 0;

    [[noreturn]] void Fail(const std::string& message) const {
        throw std::runtime_error(message + " at byte " + std::to_string(pos_));
    }
    bool Starts(const std::string& value) const { return xml_.compare(pos_, value.size(), value) == 0; }
    void Open(const std::string& value) {
        if (!Starts(value)) Fail("expected " + value);
        pos_ += value.size();
    }
    void Skip() {
        while (true) {
            while (pos_ < xml_.size() && Space(xml_[pos_])) ++pos_;
            if (!Starts("<!--")) break;
            const size_t end = xml_.find("-->", pos_ + 4);
            if (end == std::string::npos) Fail("unfinished comment");
            pos_ = end + 3;
        }
    }
    std::string Attribute() {
        Open("name=");
        if (pos_ >= xml_.size() || (xml_[pos_] != '"' && xml_[pos_] != '\'')) Fail("expected quoted name");
        const char quote = xml_[pos_++];
        const size_t end = xml_.find(quote, pos_);
        if (end == std::string::npos) Fail("unfinished name");
        const std::string name = XmlText(xml_.substr(pos_, end - pos_));
        pos_ = end + 1;
        if (name.empty() || name == "." || name == ".." || name.find('/') != std::string::npos ||
            name.find('\\') != std::string::npos || name.find('\0') != std::string::npos) Fail("invalid name");
        return name;
    }
    void ReadChildren(VfsNode& parent, const std::string& tag, int depth) {
        if (depth > 64) Fail("too many directory levels");
        while (true) {
            Skip();
            if (Starts("</" + tag + ">")) {
                pos_ += tag.size() + 3;
                return;
            }
            if (Starts("<dir ")) {
                pos_ += 5;
                VfsNode child;
                child.name = Attribute();
                Open(">");
                ReadChildren(child, "dir", depth + 1);
                Add(parent, std::move(child));
            } else if (Starts("<file ")) {
                pos_ += 6;
                VfsNode child;
                child.directory = false;
                child.name = Attribute();
                bool binary = false;
                if (Starts(" encoding=\"base64\"")) {
                    pos_ += std::string(" encoding=\"base64\"").size();
                    binary = true;
                }
                Open(">");
                const size_t end = xml_.find("</file>", pos_);
                if (end == std::string::npos) Fail("unfinished file");
                const std::string value = xml_.substr(pos_, end - pos_);
                if (value.find('<') != std::string::npos) Fail("unexpected tag in file");
                child.data = binary ? Base64(value) : XmlText(value);
                pos_ = end + 7;
                Add(parent, std::move(child));
            } else Fail("expected <dir>, <file> or closing tag");
        }
    }
    void Add(VfsNode& parent, VfsNode child) {
        for (const auto& item : parent.children)
            if (item.name == child.name) Fail("duplicate name: " + child.name);
        parent.children.push_back(std::move(child));
    }
};

void DumpNode(const VfsNode& node, const std::string& path, std::ostream& out) {
    for (const auto& child : node.children) {
        const std::string child_path = path + child.name;
        if (child.directory) {
            out << child_path << "/\n";
            DumpNode(child, child_path + "/", out);
        } else out << child_path << " (" << child.data.size() << " bytes)\n";
    }
}

} // namespace

void Vfs::Load(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open file: " + path);
    const std::string xml((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad()) throw std::runtime_error("cannot read file: " + path);
    VfsNode next = XmlParser(xml).Parse();
    root_ = std::move(next);
    cwd_.clear();
    loaded_ = true;
}

void Vfs::Dump(std::ostream& out) const {
    out << "/\n";
    DumpNode(root_, "/", out);
}

const VfsNode* Vfs::Resolve(const std::string& path, std::vector<std::string>& names,
                            std::string& error) const {
    if (!loaded_) {
        error = "no VFS loaded";
        return nullptr;
    }
    names = path.empty() || path[0] != '/' ? cwd_ : std::vector<std::string>{};
    std::vector<const VfsNode*> stack{&root_};
    for (const auto& name : names) {
        const VfsNode* next = nullptr;
        for (const auto& child : stack.back()->children)
            if (child.name == name) next = &child;
        stack.push_back(next); // cwd_ всегда указывает на существующую папку.
    }
    std::istringstream input(path);
    std::string part;
    while (std::getline(input, part, '/')) {
        if (part.empty()) continue;
        if (part == "." && stack.back()->directory) continue;
        if (!stack.back()->directory) {
            error = "not a directory";
            return nullptr;
        }
        if (part == "..") {
            if (stack.size() > 1) {
                stack.pop_back();
                names.pop_back();
            }
            continue;
        }
        const VfsNode* next = nullptr;
        for (const auto& child : stack.back()->children)
            if (child.name == part) next = &child;
        if (!next) {
            error = "no such file or directory";
            return nullptr;
        }
        names.push_back(part);
        stack.push_back(next);
    }
    if (path.size() > 1 && path.back() == '/' && !stack.back()->directory) {
        error = "not a directory";
        return nullptr;
    }
    return stack.back();
}

const VfsNode* Vfs::Find(const std::string& path, std::string& error) const {
    std::vector<std::string> names;
    return Resolve(path, names, error);
}

bool Vfs::ChangeDirectory(const std::string& path, std::string& error) {
    std::vector<std::string> names;
    const VfsNode* node = Resolve(path, names, error);
    if (!node) return false;
    if (!node->directory) {
        error = "not a directory";
        return false;
    }
    cwd_ = std::move(names);
    return true;
}

std::string Vfs::Pwd() const {
    std::string path = "/";
    for (const auto& name : cwd_) {
        if (path != "/") path += '/';
        path += name;
    }
    return path;
}
