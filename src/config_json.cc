#include "config_json.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "config.h"
namespace sylar
{

ConfigVarBaseJson::ptr ConfigJson::LookupJsonBase(const std::string &name)
{
    RWMutexType::ReadLock lock(GetMutex());
    auto it = GetDatas().find(name);
    return it == GetDatas().end() ? nullptr : it->second;
}

static void ListAllMemberJson(const std::string &prefix,
                              const nlohmann::json &node,
                              std::list<std::pair<std::string, const nlohmann::json&>> &output) {
    if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
        return;
    }
    if(node.is_null())
    {
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "ListAllMemberJson: " << prefix << " : " << node;

    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            if(it.value().is_object())
            {
                ListAllMemberJson(prefix.empty() ? it.key() : prefix + "." + it.key(), it.value(), output);
            }
            
        }
    } else if (node.is_array()) {
        for (size_t i = 0; i < node.size(); ++i) {
            if(node[i].is_object())
            {
                ListAllMemberJson(prefix + "[" + std::to_string(i) + "]", node[i], output);
            }
        }
    }
    // 对于基本数据类型，不需要进一步递归，因为它们没有子成员
}

struct KeyValuePair {
    std::string key;
    nlohmann::json value;
};

static void collectKeyValuePairs(const nlohmann::json& obj, const std::string& prefix, std::vector<KeyValuePair>& pairs) {
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

void ConfigJson::LoadFromJson(const nlohmann::json &root)
{
    // std::list<std::pair<std::string, const nlohmann::json&>> all_member;
    // ListAllMemberJson("", root, all_member);
    std::vector<KeyValuePair> all_member;
    collectKeyValuePairs(root, "", all_member);

    for(auto &i : all_member){
        std::string key = i.key;
        if(key.empty())
        {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBaseJson::ptr v = LookupJsonBase(key);
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "LookupJsonBase: " << key << " : " << i.value << std::endl;
        
        if(v)
        {
            if(i.value.is_string())
            {
                v->fromString(i.value);
            }
            else{
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config fromString, not support type: " << i.value << std::endl;
            }
        }
    }
}

static std::map<std::string, uint64_t> s_file_json_modify_time;

static sylar::Mutex s_json_mutex;

static nlohmann::json load_json_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    return nlohmann::json::parse(file);
}

void ConfigJson::LoadFromConfigJsonDir(const std::string &path, bool force)
{
    std::string absoulte_path = sylar::EnvMgr::GetInstance()->getAbsolutePath(path);
    std::vector<std::string> files;
    FSUtil::ListAllFile(files, absoulte_path, ".json");
    for(auto &i : files)
    {
        {
            struct stat st;
            lstat(i.c_str(), &st);
            sylar::Mutex::Lock lock(s_json_mutex);
            if(!force && s_file_json_modify_time[i] == (uint64_t)st.st_mtime)
            {
                continue;
            }
            s_file_json_modify_time[i] = st.st_mtime;
        }
        try{
            nlohmann::json j = load_json_from_file(i);
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "load_json_from_file: " << j << std::endl;
            LoadFromJson(j);
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "LoadJsonFile file=" << i << " success" << std::endl;
        }
        catch(...)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "LoadJsonFile file=" << i << " failed" << std::endl;
        }
    }
}

void ConfigJson::Visit(std::function<void(ConfigVarBaseJson::ptr)> cb)
{
    RWMutexType::ReadLock lock(GetMutex());
    ConfigJsonVarMap &m = GetDatas();
    for(auto &i : m)
    {
        cb(i.second);
    }
}

}

