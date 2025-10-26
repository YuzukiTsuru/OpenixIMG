#include "OpenixUtils.hpp"
#include <iostream>
using namespace OpenixIMG;

// Initialize static member
bool OpenixUtils::verboseEnabled_ = false;

void OpenixUtils::setVerboseEnabled(bool enabled) {
    verboseEnabled_ = enabled;
}

bool OpenixUtils::isVerboseEnabled() {
    return verboseEnabled_;
}

void OpenixUtils::log(const std::string &message) {
    if (verboseEnabled_) {
        std::cout << message << std::endl;
    }
}
