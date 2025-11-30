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
#include <limits>

#include <system/platform.hpp>

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
 * 256 (2^8) allows us to use bit-shifts (>> 8) instead of division.
 * It also provides better precision than 100. 
 *
 * @author Alexander Loodin Ek
 */

constexpr int WEIGHT_SCALE = 256;

// TODO: Rewrite this to only use integers, no need for templates.

// ----------------------------------------------------------------------------
// Deserialization of weight config
// ----------------------------------------------------------------------------

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

// Helper: Safely parse string to integer type
template<typename T>
T fromString(const std::string& s) {
    // ENFORCEMENT: We do not accept floating point types for weights anymore.
    // Use fixed point integers (e.g. 150 instead of 1.5).
    static_assert(std::is_integral_v<T>, "Weights must be integer types");

    size_t pos;
    // std::stoll handles negative numbers correctly
    long long val = std::stoll(s, &pos); 
    
    if (pos != s.length()) {
        throw ephant::io_error("Invalid integer format: " + s);
    }
    
    // Check for overflow if T is smaller than long long (e.g. i16)
    if (val < static_cast<long long>(std::numeric_limits<T>::min()) ||
        val > static_cast<long long>(std::numeric_limits<T>::max())) {
        throw ephant::io_error("Integer value out of range for target type: " + s);
    }
    return static_cast<T>(val);
}

class WeightStore;

// ----------------------------------------------------------------------------
// Weight Interface
// ----------------------------------------------------------------------------
class IWeight {
public:
    friend class WeightStore;
    IWeight(std::string name) :
        m_name(name) {};
    virtual ~IWeight() = default;

    virtual void accept(WeightStore& store, const std::string& newValue) = 0;
    const std::string& readName() const { return m_name; }

protected:
    std::string m_name;
};

// ----------------------------------------------------------------------------
// Tapered Weight (Fixed Point Implementation)
// ----------------------------------------------------------------------------
template<typename T>
class TaperedWeight : IWeight {
    static_assert(std::is_integral_v<T>, "TaperedWeight requires integer type");

public:
    TaperedWeight(std::string name, T& a, T& b);

    virtual void accept(WeightStore& store, const std::string& newValue) override;
    
    [[nodiscard]] T operator*(float phase0to1) const {
        // We accept a float 'phase' (0.0 to 1.0) for API compatibility, but immediately convert to fixed point.
        const T phase = static_cast<T>(phase0to1 * WEIGHT_SCALE);
        
        // Linear Interpolation: A + (B - A) * t
        // We use (Value * Phase) >> 8 to divide by 256
        int result = m_a + ((m_b - m_a) * phase) / WEIGHT_SCALE;
        return static_cast<T>(result);
    }

    [[nodiscard]] T interpolate(int phase0to256) const {
        // Overload for when you already have a fixed point phase (0-256)
        int result = m_a + ((m_b - m_a) * phase0to256) / WEIGHT_SCALE;
        return static_cast<T>(result);
    }

private:
    friend class WeightStore;
    T& m_a; // opening value
    T& m_b; // endgame value
};

// ----------------------------------------------------------------------------
// Simple Weight
// ----------------------------------------------------------------------------
template<typename T>
class Weight : IWeight {
    static_assert(std::is_integral_v<T>, "Weight requires integer type");
public:
    Weight(std::string name, T& value);

    virtual void accept(WeightStore& store, const std::string& newValue) override;

private:
    friend class WeightStore;
    T& m_value;
};

// ----------------------------------------------------------------------------
// Multiplier Weight (User writes "1.5", We store "1536")
// ----------------------------------------------------------------------------
class MultiplierWeight : public IWeight {
public:
    // We use a larger scale (1024) for precision.
    static constexpr i16 SCALE = 1024; 

    MultiplierWeight(std::string name, i16& storage);

    // The Parser: Reads "1.5", stores integer
    void accept(WeightStore& store, const std::string& newValue) override;

    // Operator Overload
    friend i16 operator*(i16 value, const MultiplierWeight& weight) {
        // Cast to i64 to prevent overflow before division
        return static_cast<i16>((static_cast<i64>(value) * weight.m_storage) / SCALE);
    }

    // Commutative property (weight * value)
    friend i16 operator*(const MultiplierWeight& weight, i16 value) {
        return value * weight;
    }

    // Allow implicit read of the raw fixed-point value if needed
    operator i16() const { return m_storage; }

private:
    friend class WeightStore;
    i16& m_storage; // Reference to the actual integer variable in evaluator_data
};

// ----------------------------------------------------------------------------
// Store
// ----------------------------------------------------------------------------
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
    
    void visit(MultiplierWeight& weight, const std::string& newValue);

private:
    static WeightStore* instance;
    WeightStore() = default;

    FileReader* m_fileReader;
    std::unordered_map<std::string, IWeight*> m_weights;
};

// ----------------------------------------------------------------------------
// Template Implementations
// ----------------------------------------------------------------------------
template<typename T>
void WeightStore::visit(Weight<T>& weight, const std::string& newValueStr) {
    weight.m_value = fromString<T>(newValueStr);
}

template<typename T>
void WeightStore::visit(TaperedWeight<T>& weight, const std::string& a, const std::string& b) {
    // might throw
    T new_a = fromString<T>(a);
    T new_b = fromString<T>(b);

    // commit
    weight.m_a = new_a;
    weight.m_b = new_b;
}

// implementation
template<typename T>
Weight<T>::Weight(std::string name, T& value) 
    : IWeight(std::move(name))
    , m_value(value)
{
    WeightStore::get()->book(this);
}

template<typename T>
void Weight<T>::accept(WeightStore& store, const std::string& newValue) 
{
    store.visit(*this, newValue);
}

template<typename T>
TaperedWeight<T>::TaperedWeight(std::string name, T& a, T& b) 
    : IWeight(name)
    , m_a(a)
    , m_b(b)
{
    WeightStore::get()->book(this);
}

template<typename T>
void TaperedWeight<T>::accept(WeightStore& store, const std::string& newValue) 
{    
    std::stringstream ss(newValue);
    std::string a, b;
    ss >> a >> b;
    store.visit(*this, a, b);
}

// ----------------------------------------------------------------------------
// Macros
// ----------------------------------------------------------------------------
#define STRINGIZE(x) #x
#define WEIGHT(name, type, defaultValue) \
    type name = defaultValue;  \
    Weight<type> weight_##name(STRINGIZE(name), name);

#define TAPERED_WEIGHT(name, type, a, b) \
    type name##_a = a; \
    type name##_b = b; \
    TaperedWeight<type> name(STRINGIZE(name), name##_a, name##_b);

#define MULTIPLIER(name, defaultValue) \
    i16 name = static_cast<i16>(defaultValue * 1024); \
    MultiplierWeight weight_##name(STRINGIZE(name), name);