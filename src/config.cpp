#include "config.h"
#include "utils.h"

#include <queue>

namespace qff {

ConfigVar::ConfigVar() {
}

ConfigVar::ConfigVar(const std::string& key)
    :key(key) {
}

ConfigVar::ConfigVar(const std::string& key, const std::string& value) 
    :key(key)
    ,val(value) {
}

Config::Config() {
}

void Config::load_from_path(const std::string& path) {
    std::vector<std::string> files;
    files = FSUtils::ListAllFile(path);
    for(auto i : files) {
        this->load_from_file(i);
    }
}

static void __load_from_node(Config::NodesType& config_vars, Config::NodeType::ptr config_var,const YAML::Node& node) {
    switch(node.Type()) {
        case YAML::NodeType::Map:{
            for (auto base_it = node.begin(); base_it != node.end(); ++base_it) {
                const std::string& key = base_it->first.Scalar();
                YAML::Node next_node = base_it->second;
                Config::NodeType::ptr new_config_var 
                    = std::make_shared<Config::NodeType>(key);
                __load_from_node(new_config_var->nexts, new_config_var, next_node);
                config_vars.push_back(new_config_var);
            }
            break;
        }
        case YAML::NodeType::Scalar: {
            config_var->val = node.Scalar();
            config_var->type = Config::NodeType::STRING;
            break;
        }
        case YAML::NodeType::Sequence: {
            config_var->type = Config::NodeType::LIST;
            for(auto it = node.begin(); it != node.end(); ++it) {
                Config::NodeType::ptr new_config_var =
                    std::make_shared<Config::NodeType>("<sequence>", it->as<std::string>());
                config_var->nexts.push_back(new_config_var);
            }
            break;
        }
        default:
            return;
    }
} //__load_from_node

void Config::load_from_file(const std::string& file_name) {
    YAML::Node node = GetYamlFromFile(file_name);
    __load_from_node(m_roots, nullptr, node);
}

static void __get_var(const Config::NodesType& nodes, std::queue<std::string>& keys, std::string& result) {
    if(keys.empty()) return;
    if(keys.size() == 1) {
        for(auto i : nodes) {
            if(i->key == keys.front()) {
                result = i->val;
                return;
            }
        }
        return;
    }
    for(auto i : nodes) {
        if(i->key == keys.front()) {
            keys.pop();
            __get_var(i->nexts, keys, result);
        }
    }
}
    
int Config::get_int_var(const std::string& key) noexcept {
    std::string result_s = std::move(this->get_string_var(key));
    int result = std::atoi(result_s.c_str());
    return result;
}

std::string Config::get_string_var(const std::string& key) noexcept {
    std::string result;
    std::vector<std::string> keys = StringUtils::Spilt(key, '.');
    std::queue<std::string> q_keys;
    for(const auto& i : keys)
        q_keys.push(i);
    __get_var(m_roots, q_keys, result);
    return result;
}

static void __get_vec_var(const Config::NodesType& nodes, std::queue<std::string>& keys, std::vector<std::string>& result) {
    if(keys.empty()) return;
    if(keys.size() == 1) {
        for(auto i : nodes) {
            if(i->key == keys.front()) {
                for(const auto& it : i->nexts) {
                    result.push_back(it->val);
                }
                return;
            }
        }
        return;
    }
    for(auto i : nodes) {
        if(i->key == keys.front()) {
            keys.pop();
            __get_vec_var(i->nexts, keys, result);
        }
    }
}

std::vector<std::string> Config::get_vec_string_var(const std::string& key) {
    std::vector<std::string> result;
    std::vector<std::string> keys = StringUtils::Spilt(key, '.');
    std::queue<std::string> q_keys;
    for(const auto& i : keys)
        q_keys.push(i);
    __get_vec_var(m_roots, q_keys, result);
    return result;
}

std::vector<int> Config::get_vec_int_var(const std::string& key) {
    std::vector<std::string> result = this->get_vec_string_var(key);
    std::vector<int> i_result;
    for (const auto& i : result) {
        i_result.push_back(std::atoi(i.c_str()));
    }
    return i_result;
}

static void __out_put(std::ostream& os, const Config::NodesType& nodes, const std::string& prefix = "") {
    for(auto i : nodes) {
        os << prefix << i->key << " : " << i->val << '\n';
        if(!i->nexts.empty()) {
            __out_put(os, i->nexts, prefix + "  ");
        }
    }
}

std::ostream& operator<<(std::ostream& os, const Config& config) {
    __out_put(os, config.m_roots);
    return os;
}

}