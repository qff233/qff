#include "log.h"

int main() {
    qff::LoggerMgr::New();

    //YAML::Node node = qff::GetYamlFromFile("test.yaml");


    QFF_LOG_DEBUG(QFF_LOG_ROOT) << "123";

    qff::LoggerMgr::Delete();
    return 0;
}