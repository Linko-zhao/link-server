#include "../linko/config.h"
#include "../linko/log.h"
#include <yaml-cpp/yaml.h>

#include <iostream>

linko::ConfigVar<int>::ptr g_int_value_config = 
    linko::Config::Lookup("system.port", (int)8080, "system port");

//linko::ConfigVar<float>::ptr g_int_valuex_config = 
//    linko::Config::Lookup("system.port", (float)8080, "system port");

linko::ConfigVar<float>::ptr g_float_value_config = 
    linko::Config::Lookup("system.value", (float)19.1, "system value");

linko::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
    linko::Config::Lookup("system.int_vec", std::vector<int>{1, 5}, "system int vec");

linko::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
    linko::Config::Lookup("system.int_list", std::list<int>{10, 20}, "system int list");

linko::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
    linko::Config::Lookup("system.int_set", std::set<int>{30, 40}, "system int set");

linko::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = 
    linko::Config::Lookup("system.int_uset", std::unordered_set<int>{50, 60}, "system int uset");

linko::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config = 
    linko::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"f", 4}}, "system int map");

linko::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config = 
    linko::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 5}}, "system int umap");

void print_yaml (const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << std::string(level * 4, ' ') 
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsNull()) {
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << std::string(level * 4, ' ') 
            << "NULL - " << node.Type() << " - " << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LINKO_LOG_INFO(LINKO_LOG_ROOT()) << std::string(level * 4, ' ') 
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            LINKO_LOG_INFO(LINKO_LOG_ROOT()) << std::string(level * 4, ' ') 
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/linko/Code/link-server/bin/conf/test.yml");
    //LINKO_LOG_INFO(LINKO_LOG_ROOT()) << root;
    print_yaml(root, 0);
}

void test_config() {
    LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "before:" << g_int_value_config->getValue();
    LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "before:" << g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto& i : v) { \
            LINKO_LOG_INFO(LINKO_LOG_ROOT()) << #prefix " " #name " : " << i; \
        } \
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << #prefix " " #name " yaml : \n" << g_var->toString(); \
    }

#define XX_M(g_var, name, prefix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto& i : v) { \
            LINKO_LOG_INFO(LINKO_LOG_ROOT()) << #prefix " " #name " : {" \
                << i.first << " - " << i.second << "}"; \
        } \
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << #prefix " " #name " yaml : \n" << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, std_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/linko/Code/link-server/bin/conf/test.yml");
    linko::Config::LoadFromYaml(root);

    LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "after:" << g_int_value_config->getValue();
    LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "after:" << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, std_int_umap, after);
}

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
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace linko {
template<>
class LexicalCast<std::string, Person> {
public:
    Person operator ()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person val;
        val.m_name = node["name"].as<std::string>();
        val.m_age = node["age"].as<int>();
        val.m_sex = node["sex"].as<bool>();
        return val;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator ()(const Person& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        node["name"] = v.m_name;
        node["age"] = v.m_age;
        node["sex"] = v.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

linko::ConfigVar<Person>::ptr g_person = 
    linko::Config::Lookup("class.person", Person(), "system person");
linko::ConfigVar<std::map<std::string, Person> >::ptr g_person_map = 
    linko::Config::Lookup("class.map", std::map<std::string, Person>(), "system map");

void test_class() {
    LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "before" << g_person->getValue().toString() << " - " << g_person->toString();

#define XX_CM(g_var, prefix) \
    { \
        auto m = g_var->getValue(); \
        for (auto& i : m) { \
            LINKO_LOG_INFO(LINKO_LOG_ROOT()) << prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << prefix << ": size = " << m.size(); \
    }

    g_person->addListener([](const Person& old_value, const Person& new_value){
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "old value : " << old_value.toString()
            << " new value : " << new_value.toString();
    });

    XX_CM(g_person_map, "class.map before ");
    YAML::Node root = YAML::LoadFile("/home/linko/Code/link-server/bin/conf/test.yml");
    linko::Config::LoadFromYaml(root);

    LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "after" << g_person->getValue().toString() << " - " << g_person->toString();
    XX_CM(g_person_map, "class.map after ");
}

void test_log() {
    static linko::Logger::ptr system_log = LINKO_LOG_NAME("system");
    LINKO_LOG_INFO(system_log) << "hello system" << std::endl;
    std::cout << linko::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/linko/Code/link-server/bin/conf/log.yml");
    linko::Config::LoadFromYaml(root);
    std::cout << "============================" << std::endl;
    std::cout << linko::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    LINKO_LOG_INFO(system_log) << "hello system" << std::endl;
    system_log->setFormatter("%d - %m%n");
    //std::cout << "============================" << std::endl;
    //std::cout << linko::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    LINKO_LOG_INFO(system_log) << "hello system" << std::endl;
}

int main(int argc, char** argv) {
    //test_yaml();
    //test_config();
    //test_class();
    test_log();

    linko::Config::Visit([](linko::ConfigVarBase::ptr var) {
        LINKO_LOG_INFO(LINKO_LOG_ROOT()) << "name=" << var->getName()
            << " description=" << var->getDescription()
            << " typename=" << var->getTypeName()
            << " value=" << var->toString();
    });
    return 0;
}
