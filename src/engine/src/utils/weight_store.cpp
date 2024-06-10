#include "utils/weight_store.hpp"

#include <fstream>
#include <sstream>

#include "log.h"

static WeightStore instance;

void WeightStore::book(IWeight* weight) {
    m_weights[weight->readName()] = weight;
}

void WeightStore::loadFromFile(const std::string& filename)  {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR() << "Error: Could not open config file " << filename;
        return;
    }

    template <typename T>
void setParamValue(const std::string& paramName, void* paramPtr, const std::string& valueStr) {
    T* typedPtr = static_cast<T*>(paramPtr);
    std::istringstream(valueStr) >> *typedPtr;  
}

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string paramName, equalsSign, valueAStr, valueBStr;
        ss >> paramName >> equalsSign >> valueAStr >> valueBStr;

        // Check for valid syntax
        if (equalsSign != "=") {
            LOG_ERROR()  << "Warning: Invalid syntax in config file on line: " << line;
            continue;
        }

        // Find the parameter and set its value (using templates)
        auto it = m_weights.find(paramName);
        if (it != m_weights.end()) {
            void* paramPtr = it->second;
            if (paramPtr) {
                try {
                    setParamValue(paramName, paramPtr, valueStr);
                } catch (const std::invalid_argument& e) {
                    LOG_ERROR() << "Error setting parameter '" << paramName << "': " << e.what();
                }
            }
        } else {
            LOG_ERROR() << "Warning: Unknown parameter in config file: " << paramName;
        }
    }
}