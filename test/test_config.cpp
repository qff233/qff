#include "log.h"
#include "utils.h"
#include "config.h"

int main() {
    qff::LoggerMgr::New();
    qff::ConfigMgr::New();

    // YAML::Node node = qff::GetYamlFromFile("test.yaml");
    // QFF_LOG_DEBUG(QFF_LOG_ROOT) << node.Type();
    // QFF_LOG_DEBUG(QFF_LOG_ROOT) << node.begin()->first;
    // QFF_LOG_DEBUG(QFF_LOG_ROOT) << node.begin()->second;

    //YAML::Node node1 = node.begin()->second;
    qff::Config* config = qff::ConfigMgr::Get();
    config->load_from_file("test.yaml");

    QFF_LOG_DEBUG(QFF_LOG_ROOT) << *config;

    QFF_LOG_DEBUG(QFF_LOG_ROOT) << config->get_string_var("children.cucu");

    std::vector<std::string> a = config->get_vec_string_var("jianzhang");
    std::vector<int> b = config->get_vec_int_var("jianzhang");
    for(const auto& i : a) {
        QFF_LOG_DEBUG(QFF_LOG_ROOT) << i;
    }
    for(auto i : b) {
        QFF_LOG_DEBUG(QFF_LOG_ROOT) << i;
    }


    qff::LoggerMgr::Delete();
    qff::ConfigMgr::Delete();
    return 0;
}