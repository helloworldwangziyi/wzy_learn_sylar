#include "config_json.h"
#include "log.h"
#include <fstream>
#include <nlohmann/json.hpp>

nlohmann::json load_json_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    return nlohmann::json::parse(file);
}


// sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_config_json()
{
    sylar::ConfigVarJson<int>::ptr g_int = sylar::ConfigJson::Lookup("test.test_int", 30);
    std::cout << g_int->getValue() << std::endl;

    sylar::ConfigVarJson<std::vector<int>>::ptr g_vec_int = sylar::ConfigJson::Lookup("test.test_vec_int", std::vector<int>{1, 2, 3});
    std::cout << g_vec_int->getValue().size() << std::endl;

    std::cout << g_vec_int->toString() << std::endl;

    g_vec_int->addListener([](const std::vector<int> &old_value, const std::vector<int> &new_value){
        std::cout << "old_value: ";
        for(auto &i : old_value) {
            std::cout << i << " ";
        }
        std::cout << "new_value: ";
    });

    // sylar::ConfigVarJson<std::string>::ptr g_string = sylar::ConfigJson::Lookup("test.test_string", "hello");
    // std::cout << g_string->getValue() << std::endl;
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_string->getValue();
    auto a = sylar::LexicalCastJson<std::string, std::map<std::string, int>>()("{\"key1\": 1, \"key2\": 2}");
    //std::map<std::string, int> b = sylar::LexicalCastJson<std::map<std::string, int>, std::string>()(a);
    std::cout << a["key1"] << std::endl;
    // std::cout << b["key1"] << std::endl;
    // std::cout << b["key2"] << std::endl;



    sylar::ConfigVarJson<std::map<std::string, int>>::ptr g_map_string2int = sylar::ConfigJson::Lookup("global.map_string2int", std::map<std::string, int>{{"key1", 1}, {"key2", 2}}, "global map string2int");
    std::cout << g_map_string2int->getValue().size() << std::endl;
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_map_string2int->toString();


    sylar::ConfigVarJson<std::unordered_map<std::string, int>>::ptr g_unordered_map_string2int = sylar::ConfigJson::Lookup("global.unordered_map_string2int", std::unordered_map<std::string, int>{{"key1", 1}, {"key2", 2}}, "global unordered_map string2int");
    std::cout << g_unordered_map_string2int->getValue().size() << std::endl;
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_unordered_map_string2int->toString();
    
}

////////////////////////////////////////////////////////////
// 自定义配置
class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;
    
    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           <<"]";
        return ss.str();
    }

    bool operator==(const Person &oth) const {
        return m_name == oth.m_name && m_age == oth.m_age && m_sex == oth.m_sex;
    }
};
// 实现自定义配置的YAML序列化与反序列化，这部分要放在sylar命名空间中
namespace sylar {

template<>
class LexicalCastJson<std::string, Person> {
public:
    Person operator()(const std::string &v) {
        nlohmann::json j = nlohmann::json::parse(v);
        Person p;
        p.m_name = j["name"].get<std::string>();
        p.m_age = j["age"].get<int>();
        p.m_sex = j["sex"].get<bool>();
        return p;
    }
};

template<>
class LexicalCastJson<Person, std::string> {
public:
    std::string operator()(const Person &p) {
        nlohmann::json j;
        j["name"] = p.m_name;
        j["age"] = p.m_age;
        j["sex"] = p.m_sex;
        return j.dump();
    }
};

} // end namespace sylar

sylar::ConfigVarJson<Person>::ptr g_person_json = 
    sylar::ConfigJson::Lookup("class.person", Person(), "system person");

sylar::ConfigVarJson<std::map<std::string, Person>>::ptr g_person_map_json = 
    sylar::ConfigJson::Lookup("class.map", std::map<std::string, Person>(), "system person map");

sylar::ConfigVarJson<std::map<std::string, std::vector<Person>>>::ptr g_person_vec_map_json = 
    sylar::ConfigJson::Lookup("class.vec_map", std::map<std::string, std::vector<Person>>(), "system vec map");

void test_class() {
    static uint64_t id = 0;

    if(!g_person_json->getListener(id)) {
        id = g_person_json->addListener([](const Person &old_value, const Person &new_value){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "g_person value change, old value:" << old_value.toString()
                << ", new value:" << new_value.toString();
        });
    }

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_person_json->getValue().toString();

    // 创建map
    std::map<std::string, Person> persons;
    persons["p1"] = Person();
    persons["p1"].m_name = "p1";
    persons["p1"].m_age = 10;
    
    persons["p2"] = Person();
    persons["p2"].m_name = "p2";
    persons["p2"].m_age = 20;

    g_person_map_json->setValue(persons);

    for (const auto &i : g_person_map_json->getValue()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i.first << ":" << i.second.toString();
    }

    // for(const auto &i : g_person_vec_map_json->getValue()) {
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i.first;
    //     for(const auto &j : i.second) {
    //         SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << j.toString();
    //     }
    // }

    std::map<std::string, std::vector<Person>> persons_vec;
    persons_vec["p1"] = std::vector<Person>{Person(), Person()};
    persons_vec["p2"] = std::vector<Person>{Person(), Person()};
    g_person_vec_map_json->setValue(persons_vec);
    for(const auto &i : g_person_vec_map_json->getValue()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i.first;
        for(const auto &j : i.second) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << j.toString();
        }
    }
}

void ListAllMemberJson(const std::string &prefix, const nlohmann::json &node, std::list<std::pair<std::string, const nlohmann::json&>> &output) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string new_prefix = prefix.empty() ? it.key() : prefix + "." + it.key();
            ListAllMemberJson(new_prefix, it.value(), output);
        }
    } else if (node.is_array()) {
        for (size_t i = 0; i < node.size(); ++i) {
            ListAllMemberJson(prefix + "[" + std::to_string(i) + "]", node[i], output);
        }
    } else {
        output.push_back(std::make_pair(prefix, node));
    }
}
struct KeyValuePair {
    std::string key;
    nlohmann::json value;
};

void collectKeyValuePairs(const nlohmann::json& obj, const std::string& prefix, std::vector<KeyValuePair>& pairs) {
    if (obj.is_object()) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            std::string current_key = prefix.empty() ? it.key() : prefix + "." + it.key();
            if (it->is_structured()) {
                // 如果是结构化数据，继续递归
                collectKeyValuePairs(*it, current_key, pairs);
            } else {
                // 如果是基本数据类型，保存 key-value 对
                pairs.push_back({current_key, *it});
            }
        }
    } else if (obj.is_array()) {
        for (size_t i = 0; i < obj.size(); ++i) {
            collectKeyValuePairs(obj[i], prefix + "[" + std::to_string(i) + "]", pairs);
        }
    } else {
        // 如果是基本数据类型，保存 key-value 对
        pairs.push_back({prefix, obj});
    }
}


int main(int argc, char **argv)
{
    // sylar::ConfigVarJson<int>::ptr g_int = sylar::ConfigJson::Lookup("test.test_int", 30);
    // std::cout << g_int->getValue() << std::endl;

    // sylar::ConfigVarJson<std::vector<int>>::ptr g_vec_int = sylar::ConfigJson::Lookup("test.test_vec_int", std::vector<int>{1, 2, 3});
    // std::cout << g_vec_int->getValue().size() << std::endl;

    // std::cout << g_vec_int->toString() << std::endl;

    // auto it = sylar::ConfigJson::Lookup("test.test_vec_int", std::vector<int>{1, 2, 3});
    // std::cout << it->getValue().size() << std::endl;


    // g_vec_int->addListener([](const std::vector<int> &old_value, const std::vector<int> &new_value){
    //     std::cout << "old_value: ";
    //     for(auto &i : old_value) {
    //         std::cout << i << " ";
    //     }
    //     std::cout << "new_value: ";
    // });

    // g_vec_int->setValue(std::vector<int>{4, 5, 6});
    // sylar::EnvMgr::GetInstance()->init(argc, argv);
    // sylar::ConfigJson::LoadFromConfigJsonDir("conf");
    // sylar::ConfigJson::Visit([](sylar::ConfigVarBaseJson::ptr var){
    //     std::cout << var->getName() << " = " << var->toString() << std::endl;
    // });

    // test_class();

    nlohmann::json j = load_json_from_file("conf/log.json");
    // // std::cout << j << std::endl;
    
    std::vector<KeyValuePair> output;

    collectKeyValuePairs(j, "", output);
    for(const auto &i : output) {
        std::cout << i.key << " : " << i.value << std::endl;
    }
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "================================================";
    // SYLAR_LOG_INFO(SYLAR_LOG_NAME("system")) << "================================================"; 

    // test_config_json();

    // std::cout << "output size: " << output.size() << std::endl;






    return 0;
}
