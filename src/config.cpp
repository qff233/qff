#include "config.h"
#include "utils.h"

namespace qff {

void ConfigManager::load_config_from_path(const std::string& path) {

}

void ConfigManager::load_config_from_file(const std::string& file_name) {
    YAML::Node yaml_node = GetYamlFromFile(file_name);

}

void ConfigManager::add_config(ConfigVar::ptr config_var) {
     for(const auto i : m_datas) {
        if(i->get_name() == config_var->get_name())
            return;
    }
    m_datas.push_back(config_var);
}

ConfigVar::ptr ConfigManager::get_config(const std::string& name) {
    for(const auto i : m_datas) {
        if(i->get_name() == name) 
            return i;
    }
    return nullptr;
}


}