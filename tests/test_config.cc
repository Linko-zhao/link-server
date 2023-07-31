#include "../link/config.h"
#include "../link/log.h"
#include <yaml-cpp/yaml.h>

links::ConfigVar<int>::ptr g_int_value_config = 
    links::Config::Lookup("system.port", (int)8080, "system port");

links::ConfigVar<float>::ptr g_float_value_config = 
    links::Config::Lookup("system.value", (float)19.1, "system value");

links::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
    links::Config::Lookup("system.int_vec", std::vector<int>{1, 5}, "system int vec");

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
    auto& v = g_int_vec_value_config->getValue();
    for (auto& i : v) {
        LINK_LOG_INFO(LINK_GET_ROOT()) << "int_vec: " << i;
    }

    YAML::Node root = YAML::LoadFile("/home/links/Code/link-server/bin/conf/log.yml");
    links::Config::LoadFromYaml(root);

    LINK_LOG_INFO(LINK_GET_ROOT()) << "after:" << g_int_value_config->getValue();
    LINK_LOG_INFO(LINK_GET_ROOT()) << "after:" << g_float_value_config->toString();
    auto& vec = g_int_vec_value_config->getValue();
    for (auto& i : vec) {
        LINK_LOG_INFO(LINK_GET_ROOT()) << "int_vec: " << i;
    }
}

int main(int argc, char** argv) {
    //test_yaml();
    test_config();
    return 0;
}
