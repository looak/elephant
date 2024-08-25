#include "utils/weight_store.hpp"

#include <iostream>

#include "log.h"

WeightStore* WeightStore::instance = nullptr;

WeightStore* WeightStore::get() {
    if (!instance) {
        instance = new WeightStore();
    }
    return instance;
}

void WeightStore::initialize(FileReader* fileReader) {
    m_fileReader = fileReader;
}

void WeightStore::book(IWeight* weight) {
    m_weights[weight->readName()] = weight;
}

void WeightStore::loadFromFile(const std::string& filename) {
    if (!m_fileReader->openFile(filename)) {
        LOG_ERROR() << "Could not open config file " << filename;
        return;
    }

    std::string line;
    while (m_fileReader->readLine(line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string paramName, valueAStr, valueBStr;
        ss >> paramName >> valueAStr >> valueBStr;

        // Find the parameter and set its value (using templates)
        auto it = m_weights.find(paramName);
        if (it != m_weights.end()) {
            IWeight* paramPtr = it->second;
            if (paramPtr) {
                try {
                    paramPtr->accept(*this, std::format("{} {}", valueAStr, valueBStr));
                    LOG_INFO() << "Set parameter '" << paramName << "' to " << valueAStr << " " << valueBStr;
                } catch (const std::invalid_argument& e) {
                    LOG_ERROR() << "Error setting parameter '" << paramName << "': " << e.what();
                }
            }
        } else {
            LOG_ERROR() << "Unknown parameter in config file: " << paramName;
        }
    }
}

void WeightStore::update(const std::string& name, const std::string& newValue) {
    auto it = m_weights.find(name);
    if (it != m_weights.end()) {
        it->second->accept(*this, newValue);
    }
    else {
        LOG_ERROR() << "Unknown weight name: " << name;
    }
}

// void WeightStore::visit(Weight<bool>& weight, const std::string& newValueStr) {
//     // Convert string to bool (case-insensitive)
//     weight.m_value = (newValueStr == "true" || newValueStr == "1");
// }

// void WeightStore::visit(Weight<std::string>& weight, const std::string& newValueStr) {
//     weight.m_value = newValueStr; // Directly assign the new string value
// }