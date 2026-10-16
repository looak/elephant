#include <io/weight_store.hpp>

#include <iostream>
#include <format>

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

        // Parse the line: paramName value... , one value for a weight, two for a tapered weight, a midgame and an
        // endgame table for a tapered table. The weight parses its own values.
        std::stringstream ss(line);
        std::string paramName;
        ss >> paramName;
        std::string values;
        std::getline(ss >> std::ws, values);
        // trailing spaces or a \r from a CRLF file would fail a single value's parse.
        values.erase(values.find_last_not_of(" \t\r\n") + 1);

        // Find the parameter and set its value (using templates)
        auto it = m_weights.find(paramName);
        if (it != m_weights.end()) {
            IWeight* paramPtr = it->second;
            if (paramPtr) {
                try {
                    paramPtr->accept(*this, values);
                    LOG_TRACE("Set parameter '{}' to {}", paramName, values);
                } catch (const std::exception& e) {
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