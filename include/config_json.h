#ifndef SYLAR_CONFIG_JSON_H
#define SYLAR_CONFIG_JSON_H

#include "sylar.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <functional>


namespace sylar
{
class ConfigVarBaseJson {
public:
    using ptr = std::shared_ptr<ConfigVarBaseJson>;
    ConfigVarBaseJson(const std::string& name, const std::string& description = "")
        : m_name(name), m_description(description) {}
    virtual ~ConfigVarBaseJson() {}

    const std::string& getName() const { return m_name; }
    const std::string& getDescription() const { return m_description; }

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    virtual std::string getTypeName() const = 0;

protected:
    std::string m_name;
    std::string m_description;
};


// 类型转换模板
template<class F, class T>
class LexicalCastJson {
public:
    T operator()(const F &v) {
        return boost::lexical_cast<T>(v);
    }
};



template<class T>
class LexicalCastJson<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& v)
    {
        auto j = nlohmann::json::parse(v);
         std::vector<T> vec;
        if (j.is_array()) {
           
            for (auto& i : j) {
                std::string value_str = i.dump();
                vec.push_back(LexicalCastJson<std::string, T>()(value_str));
            }
        }
        return vec;
    }
};

template<class T>
class LexicalCastJson<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        nlohmann::json j;
        for (auto& i : v) {
            j.push_back(LexicalCastJson<T, std::string>()(i));
        }
        return j.dump();
    }
};

template<class T>
class LexicalCastJson<std::string, std::list<T>>
{
public:
    std::string operator()(const std::list<T>& v) {
        nlohmann::json j;
        for (auto& i : v) {
            j.push_back(LexicalCastJson<T, std::string>()(i));
        }
        return j.dump();
    }
};

template<class T>
class LexicalCastJson<std::list<T>, std::string>
{
public:
    std::list<T> operator()(const std::string& v) {
        auto j = nlohmann::json::parse(v);
        if (j.is_array()) {
            std::list<T> vec;
            for (auto& i : j) {
                std::string value_str = i.dump();
                vec.push_back(LexicalCastJson<std::string, T>()(value_str));
            }
            return vec;
        }
        return std::list<T>();
    }
};

template<class T>
class LexicalCastJson<std::set<T>,std::string>
{
public:
    std::string operator()(const std::set<T>&v){
        nlohmann::json j;
        for (auto& i : v) {
            j.push_back(LexicalCastJson<T, std::string>()(i));
        }
        return j.dump();
    }
};

template<class T>
class LexicalCastJson<std::string, std::set<T>>
{
public:
    std::set<T> operator()(const std::string& v) {
        auto j = nlohmann::json::parse(v);
        if (j.is_array()) {
            std::set<T> vec;
            for (auto& i : j) {
                std::string value_str = i.dump();
                vec.insert(LexicalCastJson<std::string, T>()(value_str));
            }
            return vec;
        }
        return std::set<T>();
    }
};

template<class T>
class LexicalCastJson<std::unordered_set<T>,std::string>
{
public:
    std::string operator()(const std::unordered_set<T>& v) {
        nlohmann::json j;
        for (auto& i : v) {
            j.push_back(LexicalCastJson<T, std::string>()(i));
        }
        return j.dump();
    }
};

template<class T>
class LexicalCastJson<std::string, std::unordered_set<T>>
{
public:
    std::unordered_set<T> operator()(const std::string& v) {
        auto j = nlohmann::json::parse(v);
        if (j.is_array()) {
            std::unordered_set<T> vec;
            for (auto& i : j) {
                std::string value_str = i.dump();
                vec.insert(LexicalCastJson<std::string, T>()(value_str));
            }
            return vec;
        }
        return std::unordered_set<T>();
    }
};

template<class T>
class LexicalCastJson<std::map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::map<std::string, T>& v)
    {
        nlohmann::json j;
        for(auto &i : v) {
            j[i.first] = nlohmann::json::parse(LexicalCastJson<T, std::string>()(i.second));
        }
        return j.dump();
    }
};

template<class T>
class LexicalCastJson<std::string,std::map<std::string, T>>
{
public:
    std::map<std::string, T> operator()(const std::string& v) {
        auto j = nlohmann::json::parse(v);
        if (j.is_object()) {
            std::map<std::string, T> map;
            for (auto& i : j.items()) {
                std::string value_str = i.value().dump();
                map[i.key()] = LexicalCastJson<std::string, T>()(value_str);
            }
            return map;
        }
        return std::map<std::string, T>();
    }
};

template<class T>
class LexicalCastJson<std::unordered_map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::unordered_map<std::string, T>&v)
    {
        nlohmann::json j;
        for(auto &i :v)
        {
            j[i.first] =  nlohmann::json::parse(LexicalCastJson<T, std::string>()(i.second));
        }
        return j.dump();
    }
};

template<class T>
class LexicalCastJson<std::string, std::unordered_map<std::string, T>>
{
public:
    std::unordered_map<std::string, T> operator()(const std::string& v) {
        auto j = nlohmann::json::parse(v);
        std::unordered_map<std::string, T> map;
        if (j.is_object()) {
            
            for (auto& i : j.items()) {
                std::string value_str = i.value().dump();
                map[i.key()] = LexicalCastJson<std::string, T>()(value_str);
            }
            return map;
        }
        return map;
    }
};

// 配置变量模板类
template<class T, class FromStr = LexicalCastJson<std::string, T>, class ToStr = LexicalCastJson<T, std::string>>
class ConfigVarJson : public ConfigVarBaseJson {
public:
    using ptr = std::shared_ptr<ConfigVarJson>;
    using on_change_cb = std::function<void(const T& old_value, const T& new_value)>;
    using RWMutexType = RWMutex;
    ConfigVarJson(const std::string& name, const T& default_value, const std::string& description = "")
        : ConfigVarBaseJson(name, description), m_val(default_value) {}

    std::string toString() override {
        try {
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception& e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVarJson::toString, conversion failed, error: " << e.what();
        }
        return "";
    }

    bool fromString(const std::string& val) override {
        try {
            // RWMutexType::WriteLock lock(m_mutex);
            setValue(FromStr()(val));
            return true;
        } catch (std::exception& e) {
            return false;
        }
    }

    void setValue(const T &v)
    {
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(v == m_val) return;
            for(auto &cb : m_cbs)
            {
                cb.second(m_val, v);
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        m_val = v;
    }

    T getValue() { 
        RWMutexType::ReadLock lock(m_mutex);
        return m_val; 
    }


    std::string getTypeName() const override { return typeid(T).name(); }

    uint64_t addListener(on_change_cb cb) {
        static uint64_t key = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++key;
        m_cbs[key] = cb;
        return key;
    }

    void delListener(int key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

    on_change_cb getListener(int key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        if (it != m_cbs.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    mutable RWMutexType m_mutex;
    T m_val;
    std::unordered_map<int, on_change_cb> m_cbs;
};


class ConfigJson{
public:
    using ptr = std::shared_ptr<ConfigJson>;
    using ConfigJsonVarMap = std::unordered_map<std::string, ConfigVarBaseJson::ptr>;
    using RWMutexType = RWMutex;
    ConfigJson() = default;
    ~ConfigJson() = default;

    template<class T>
    static typename ConfigVarJson<T>::ptr Lookup(const std::string &name,
                                    const T&default_value, const std::string &description = "")
    {
        RWMutexType::WriteLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it != GetDatas().end())
        {
            auto tmp = std::dynamic_pointer_cast<ConfigVarJson<T>>(it->second);
            if(tmp)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVarJson::Lookup, name=" << name << " already exists";
                return tmp;
            }
            else 
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVarJson::Lookup, name=" << name << " type mismatch";
                return nullptr;
            }
        }
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._") != std::string::npos)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVarJson::Lookup, name=" << name << " invalid name";
            throw std::invalid_argument(name);
        }
        typename ConfigVarJson<T>::ptr v(new ConfigVarJson<T>(name, default_value, description));
        GetDatas()[name] = v;
        return v;  
    }

    template<class T>
    static typename ConfigVarJson<T>::ptr Lookup(const std::string &name)
    {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end())
        {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVarJson<T>>(it->second);
    }

    static void Visit(std::function<void(ConfigVarBaseJson::ptr)> cb);

    static void LoadFromJson(const nlohmann::json &root);

    static void LoadFromConfigJsonDir(const std::string &path, bool force = false);

    static  ConfigVarBaseJson::ptr LookupJsonBase(const std::string &name);
private:
    static ConfigJsonVarMap &GetDatas()
    {
        static ConfigJsonVarMap s_datas;
        return s_datas;
    }

    static RWMutexType &GetMutex()
    {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};

}

#endif