#include<iostream>
#include "sylar.h"



int main() {
    // YAML 格式的字符串
    std::string yaml_str = "[1, 2, 3, 4, 5]";

    // 使用 LexicalCast 将 YAML 字符串转换为 std::vector<int>
    sylar::LexicalCast<std::string, std::vector<int>> lexical_cast;
    std::vector<int> vec = lexical_cast(yaml_str);

    // 输出转换后的结果
    std::cout << "Converted vector: ";
    for (int i : vec) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    return 0;
}