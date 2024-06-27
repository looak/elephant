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

void WeightStore::visit(Weight<i64>& weight, const std::string& newValueStr) {
    // Attempt to convert string to int 
    try {
        weight.m_value = std::stoi(newValueStr);
    }
    catch (const std::exception& e) {
        // Handle invalid input (e.g., not an integer)
        LOG_ERROR() << "Error updating int weight: " << e.what() << std::endl;
    }
}

void WeightStore::visit(Weight<i32>& weight, const std::string& newValueStr) {
    // Attempt to convert string to int 
    try {
        weight.m_value = std::stoi(newValueStr);
    }
    catch (const std::exception& e) {
        // Handle invalid input (e.g., not an integer)
        LOG_ERROR() << "Error updating int weight: " << e.what() << std::endl;
    }
}

void WeightStore::visit(Weight<double>& weight, const std::string& newValueStr) {
    // Attempt to convert string to double
    try {
        weight.m_value = std::stod(newValueStr);
    }
    catch (const std::exception& e) {
        // Handle invalid input (e.g., not a double)
        LOG_ERROR() << "Error updating double weight: " << e.what() << std::endl;
    }
}

void WeightStore::visit(Weight<bool>& weight, const std::string& newValueStr) {
    // Convert string to bool (case-insensitive)
    weight.m_value = (newValueStr == "true" || newValueStr == "1");
}

void WeightStore::visit(Weight<std::string>& weight, const std::string& newValueStr) {
    weight.m_value = newValueStr; // Directly assign the new string value
}

void WeightStore::visit(TaperedWeight<i64>& weight, const std::string& a, const std::string& b) {
    // Attempt to convert strings to integers
    try {
        weight.m_a = std::stoi(a);
        weight.m_b = std::stoi(b);
    }
    catch (const std::exception& e) {
        // Handle invalid input (e.g., not an integer)
        LOG_ERROR() << "Error updating int weight: " << e.what() << std::endl;
    }
}
void WeightStore::visit(TaperedWeight<i32>& weight, const std::string& a, const std::string& b) {
    // Attempt to convert strings to integers
    try {
        weight.m_a = std::stoi(a);
        weight.m_b = std::stoi(b);
    }
    catch (const std::exception& e) {
        // Handle invalid input (e.g., not an integer)
        LOG_ERROR() << "Error updating int weight: " << e.what() << std::endl;
    }
}
void WeightStore::visit(TaperedWeight<double>& weight, const std::string& a, const std::string& b) {
    // Attempt to convert strings to doubles
    try {
        weight.m_a = std::stod(a);
        weight.m_b = std::stod(b);
    }
    catch (const std::exception& e) {
        // Handle invalid input (e.g., not a double)
        LOG_ERROR() << "Error updating double weight: " << e.what() << std::endl;
    }
}