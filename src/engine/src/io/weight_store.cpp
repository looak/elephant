#include <io/weight_store.hpp>

#include <iostream>

#include <diagnostics/logger.hpp>

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
        LOG_ERROR("Could not open config file {}", filename);
        return;
    }

    std::string line;
    while (m_fileReader->readLine(line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // we assume that everything is a tapered value
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
                    LOG_TRACE("Set parameter '{}' to {} {}", paramName, valueAStr, valueBStr);
                } catch (const std::invalid_argument& e) {
                    LOG_ERROR("Error setting parameter '{}': {}", paramName, e.what());
                }
            }
        } else {
            LOG_ERROR("Unknown parameter in config file: {}", paramName);
        }
    }
}

void WeightStore::update(const std::string& name, const std::string& newValue) {
    auto it = m_weights.find(name);
    if (it != m_weights.end()) {
        it->second->accept(*this, newValue);
    }
    else {
        LOG_ERROR("Unknown weight name: {}", name);
    }
}

void WeightStore::visit(MultiplierWeight& weight, const std::string& newValue) {
    try {
        // We use std::stod here because this ONLY happens at startup.
        // Performance cost is negligible.
        double userVal = std::stod(newValue);
        
        // Convert to Fixed Point: 1.5 * 1024 = 1536
        // std::round ensures 1.0 doesn't become 0.99999 -> 1023
        weight.m_storage = static_cast<i16>(std::round(userVal * MultiplierWeight::SCALE));
    } catch (...) {
        throw ephant::io_error("Invalid multiplier format: " + newValue);
    }
}

MultiplierWeight::MultiplierWeight(std::string name, i16& storage)
    : IWeight(std::move(name))
    , m_storage(storage) 
{
    WeightStore::get()->book(this);
}

void MultiplierWeight::accept(WeightStore& store, const std::string& newValue) {
    store.visit(*this, newValue);
}

// void WeightStore::visit(Weight<bool>& weight, const std::string& newValueStr) {
//     // Convert string to bool (case-insensitive)
//     weight.m_value = (newValueStr == "true" || newValueStr == "1");
// }

// void WeightStore::visit(Weight<std::string>& weight, const std::string& newValueStr) {
//     weight.m_value = newValueStr; // Directly assign the new string value
// }