// Elephant Gambit Chess Engine - a Chess AI
// Copyright(C) 2021-2023  Alexander Loodin Ek

// This program is free software : you can redistribute it and /or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.If not, see <http://www.gnu.org/licenses/>.
#pragma once

#include <fstream>
#include <unordered_map>
#include <sstream>
#include <string>

#include "defines.hpp"

/**
 * @file weight_store.hpp
 * @brief A few classes providing mechanisms for creating and updating weights.
 *
 * Weights are used in the engines evaluation function and can have a huge impact
 * on the engines performance. To allow weights to be tweakable they need to be
 * declared using one of the macros provided in the bottom of this file.
 *
 * The main usecase for these classes is to be able to update weights from a file,
 * and provide a simple interface for tweaking weights during runtime.
 *
 * @author Alexander Loodin Ek
 */

// Abstract file reader interface
class FileReader {
public:
    virtual ~FileReader() = default;
    virtual bool openFile(const std::string& filename) = 0;
    virtual bool readLine(std::string& line) = 0;
};

// Concrete implementation for file reading
class FileReaderImpl : public FileReader {
public:
    bool openFile(const std::string& filename) override {
        file.open(filename);
        return file.is_open();
    }

    bool readLine(std::string& line) override {
        if (std::getline(file, line)) {
            return true;
        }
        else {
            return false;
        }
    }

private:
    std::ifstream file;
};

template<typename T>
T fromString(const std::string& str) {
    T value;
    std::istringstream iss(str);
    iss >> value;
    return value;
}

class IWeight {
public:
    friend class WeightStore;
    IWeight(std::string name) : m_name(name) {};
    virtual ~IWeight() = default;

    virtual void accept(WeightStore& store, const std::string& newValue) = 0;

    const std::string& readName() const {
        return m_name;
    }

protected:
    std::string m_name;
};

template<typename T>
class TaperedWeight : IWeight {
public:
    TaperedWeight(std::string name, T& a, T& b)
        : IWeight(name)
        , m_a(a)
        , m_b(b)
    {
        WeightStore::get()->book(this);
    }

    virtual void accept(WeightStore& store, const std::string& newValue) override;

    i32 operator*(const float& t) {
        return static_cast<i32>(m_a + (m_b - m_a) * t);
    }

private:
    friend class WeightStore;
    T& m_a;
    T& m_b;
};

template<typename T>
class Weight : IWeight {
public:
    Weight(std::string name, T& value)
        : IWeight(name)
        , m_value(value)
    {
        WeightStore::get()->book(this);
    }

    virtual void accept(WeightStore& store, const std::string& newValue) override;

private:
    friend class WeightStore;
    T& m_value;
};

class WeightStore {
public:
    void initialize(FileReader* fileReader = new FileReaderImpl());
    static WeightStore* get();

    void book(IWeight* weight);

    void loadFromFile(const std::string& filename);
    void update(const std::string& name, const std::string& newValue);

    template<typename T>
    void visit(Weight<T>& weight, const std::string& newValueStr);

    template<typename T>
    void visit(TaperedWeight<T>& weight, const std::string& a, const std::string& b);

private:
    static WeightStore* instance;
    WeightStore() = default;

    FileReader* m_fileReader;
    std::unordered_map<std::string, IWeight*> m_weights;
};

template<typename T>
void WeightStore::visit(Weight<T>& weight, const std::string& newValueStr) {
    // Attempt to convert string to numeric type 
    try {
        weight.m_value = fromString<T>(newValueStr);
    }
    catch (const std::exception& e) {
        LOG_ERROR() << "Error updating weight: " << weight.m_name << " - " << e.what() << std::endl;
    }
}

template<typename T>
void WeightStore::visit(TaperedWeight<T>& weight, const std::string& a, const std::string& b) {
    // Attempt to convert string to numeric type 
    try {
        weight.m_a = fromString<T>(a);
        weight.m_b = fromString<T>(b);
    }
    catch (const std::exception& e) {
        LOG_ERROR() << "Error updating weight: " << weight.m_name << " - " << e.what() << std::endl;
    }
}

// implementation
template<typename T>
void Weight<T>::accept(WeightStore& store, const std::string& newValue) 
{
    store.visit(*this, newValue);
}

template<typename T>
void TaperedWeight<T>::accept(WeightStore& store, const std::string& newValue) 
{    
    std::stringstream ss(newValue);
    std::string a, b;
    ss >> a >> b;
    store.visit(*this, a, b);
}

// macros for declaring weights
#define STRINGIZE(x) #x
#define WEIGHT(name, type, defaultValue) \
type name = defaultValue;  \
Weight<type> weight_##name(STRINGIZE(name), name);

#define TAPERED_WEIGHT(name, type, a, b) \
type name##_a = a; \
type name##_b = b; \
TaperedWeight<type> name(STRINGIZE(name), name##_a, name##_b);