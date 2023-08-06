#include "../link/config.h"
#include "../link/log.h"
#include <yaml-cpp/yaml.h>

links::ConfigVar<int>::ptr g_int_value_config = 
    links::Config::Lookup("system.port", (int)8080, "system port");

links::ConfigVar<float>::ptr g_float_value_config = 
    links::Config::Lookup("system.value", (float)19.1, "system value");

links::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
    links::Config::Lookup("system.int_vec", std::vector<int>{1, 5}, "system int vec");

links::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
    links::Config::Lookup("system.int_list", std::list<int>{10, 20}, "system int list");

links::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
    links::Config::Lookup("system.int_set", std::set<int>{30, 40}, "system int set");

links::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = 
    links::Config::Lookup("system.int_uset", std::unordered_set<int>{50, 60}, "system int uset");

links::ConfigVar<std::map<std::string, int> >::ptr g_str_int_map_value_config = 
    links::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"f", 4}}, "system int map");

links::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_umap_value_config = 
    links::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k", 5}}, "system int umap");

void print_yaml (const YAML::Node& node, int level) {
    if (node.IsScalar()) {
        LINK_LOG_INFO(LINK_GET_ROOT()) << std::string(level * 4, ' ') 
            << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsNull()) {
        LINK_LOG_INFO(LINK_GET_ROOT()) << std::string(level * 4, ' ') 
            << "NULL - " << node.Type() << " - " << level;
    } else if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            LINK_LOG_INFO(LINK_GET_ROOT()) << std::string(level * 4, ' ') 
                << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) {
        for (size_t i = 0; i < node.size(); ++i) {
            LINK_LOG_INFO(LINK_GET_ROOT()) << std::string(level * 4, ' ') 
                << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/home/links/Code/link-server/bin/conf/log.yml");
    //LINK_LOG_INFO(LINK_GET_ROOT()) << root;
    print_yaml(root, 0);
}

void test_config() {
    LINK_LOG_INFO(LINK_GET_ROOT()) << "before:" << g_int_value_config->getValue();
    LINK_LOG_INFO(LINK_GET_ROOT()) << "before:" << g_float_value_config->toString();

#define XX(g_var, name, perfix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto& i : v) { \
            LINK_LOG_INFO(LINK_GET_ROOT()) << #perfix " " #name " : " << i; \
        } \
        LINK_LOG_INFO(LINK_GET_ROOT()) << #perfix " " #name " yaml : \n" << g_var->toString(); \
    }

#define XX_M(g_var, name, perfix) \
    { \
        auto& v = g_var->getValue(); \
        for (auto& i : v) { \
            LINK_LOG_INFO(LINK_GET_ROOT()) << #perfix " " #name " : {" \
                << i.first << " - " << i.second << "}"; \
        } \
        LINK_LOG_INFO(LINK_GET_ROOT()) << #perfix " " #name " yaml : \n" << g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_map_value_config, str_int_map, before);
    XX_M(g_str_int_umap_value_config, std_int_umap, before);

    YAML::Node root = YAML::LoadFile("/home/links/Code/link-server/bin/conf/log.yml");
    links::Config::LoadFromYaml(root);

    LINK_LOG_INFO(LINK_GET_ROOT()) << "after:" << g_int_value_config->getValue();
    LINK_LOG_INFO(LINK_GET_ROOT()) << "after:" << g_float_value_config->toString();

    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_map_value_config, str_int_map, after);
    XX_M(g_str_int_umap_value_config, std_int_umap, after);
}

int main(int argc, char** argv) {
    //test_yaml();
    test_config();
    return 0;
}
