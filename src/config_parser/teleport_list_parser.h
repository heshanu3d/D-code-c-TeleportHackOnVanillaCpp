#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <iomanip>
#include <functional>

struct Category;

// 铁炉堡                        :
// 铁炉堡-疯狂服                  :

struct Point {
    Point(std::string n, float fx, float fy, float fz) : name(n), x(fx), y(fy), z(fz) {
        #ifdef DEBUG
            std::cout 
                << std::setw(20) << std::left 
                << "created Point " 
                // << std::setw(20) << std::left 
                << name << std::endl;
        #endif
    }
    float x;
    float y;
    float z;
    std::string name;
    std::string ToString() { return name + ", " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z); }
    std::string ToStringWithoutName() { return std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z); }
    void Print() {
        std::cout << std::setw(10) << std::left << std::left << x    << ","
                  << std::setw(10) << std::left << std::left << y    << ","
                  << std::setw(10) << std::left << std::left << z    << ","
                  << std::setw(10) << std::left << std::left << name << std::endl;
    }
};

struct Category : public std::enable_shared_from_this<Category> {
    explicit Category(std::string name_) : name(name_), depth(0) {
        #ifdef DEBUG
            std::cout 
                << std::setw(20) << std::left 
                << "created Category " 
                // << std::setw(20) << std::left 
                << name << std::endl;
        #endif
    }
    explicit Category(std::string name_, int depth_) : name(name_), depth(depth_ + 1) {
        #ifdef DEBUG
            std::cout 
                << std::setw(20) << std::left 
                << "created Category " 
                // << std::setw(20) << std::left 
                << name << std::endl;
        #endif
    }
    ~Category() {}

    std::map<std::string, std::shared_ptr<Category>> categories;
    std::shared_ptr<Point> point = nullptr;
    std::string name;
    int depth;
    void Print() {
        #ifdef DEBUG
            std::cout << "depth-" << depth << "  ";
        #endif
        if (point)
            point->Print();
        #ifdef DEBUG
        else
            std::cout << std::endl;
        #endif
    }
    void Traverse(std::function<void(std::shared_ptr<Category>)> action = [](std::shared_ptr<Category> node){ node->Print(); });
};

class TeleportListParser {
public:
    TeleportListParser() {};
    void Init();
    std::vector<std::shared_ptr<Category>> Lists() { return m_root; }
private:
    std::shared_ptr<Category> Parse(const std::string &configFilePath);
    std::vector<std::shared_ptr<Category>> m_root;
    std::vector<std::string> m_configPaths;
    std::string config_dir = "configs\\";
};

// Category root;
