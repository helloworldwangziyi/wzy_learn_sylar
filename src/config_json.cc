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
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node << std::endl;
        return;
    }
    output.push_back(std::make_pair(prefix, node));
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            ListAllMemberJson(prefix.empty() ? it.key()
                                             : prefix + "." + it.key(),
                              it.value(), output);
        }
    }
}

void ConfigJson::LoadFromJson(const nlohmann::json &root)
{
    std::list<std::pair<std::string, const nlohmann::json&>> all_member;
    ListAllMemberJson("", root, all_member);

    for(auto &i : all_member){
        std::string key = i.first;
        if(key.empty())
        {
            continue;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        ConfigVarBaseJson::ptr v = LookupJsonBase(key);

        if(v)
        {
            if(i.second.is_string())
            {
                v->fromString(i.second.get<std::string>());
            }
            else 
            {
                std::string json_str = i.second.dump(4);  // 使用缩进为4的格式化输出
                v->fromString(json_str);
            }
        }
    }
}

static std::map<std::string, uint64_t> s_file_json_modify_time;

static sylar::Mutex s_json_mutex;

static nlohmann::json load_json_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "load_json_from_file: " << buffer.str() << std::endl;
    return nlohmann::json::parse(buffer.str());
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

