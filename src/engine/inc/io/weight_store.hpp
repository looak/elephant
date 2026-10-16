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
#include <eval/tapered.hpp>

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
// Tapered Weight, a midgame & endgame pair tapered on the game phase, see eval/tapered.hpp
// ----------------------------------------------------------------------------
template<typename T>
class TaperedWeight : IWeight {
    static_assert(std::is_integral_v<T>, "TaperedWeight requires integer type");

public:
    TaperedWeight(std::string name, Tapered<T>& value);

    virtual void accept(WeightStore& store, const std::string& newValue) override;

    // the same taper as PestoAccumulator::taperedScore(), gamePhase from maxGamePhase (midgame) to 0 (endgame).
    [[nodiscard]] T operator*(i32 gamePhase) const { return m_value * gamePhase; }

private:
    friend class WeightStore;
    Tapered<T>& m_value;
};

// ----------------------------------------------------------------------------
// Tapered Table Weight, N tapered pairs, e.g. a piece square table. Written as N midgame values then N endgame values,
// the way the tables are published.
// ----------------------------------------------------------------------------
template<typename T, std::size_t N>
class TaperedTableWeight : IWeight {
    static_assert(std::is_integral_v<T>, "TaperedTableWeight requires integer type");

public:
    TaperedTableWeight(std::string name, std::array<Tapered<T>, N>& table);

    virtual void accept(WeightStore& store, const std::string& newValue) override;

private:
    friend class WeightStore;
    std::array<Tapered<T>, N>& m_table;
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

    template<typename T, std::size_t N>
    void visit(TaperedTableWeight<T, N>& weight, const std::string& newValues);
    
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
    weight.m_value = { new_a, new_b };
}

template<typename T, std::size_t N>
void WeightStore::visit(TaperedTableWeight<T, N>& weight, const std::string& newValues) {
    // N midgame values then N endgame values, all parsed before any is committed.
    std::stringstream ss(newValues);
    std::array<T, 2 * N> values{};
    for (std::size_t i = 0; i < 2 * N; ++i) {
        std::string token;
        if (!(ss >> token))
            throw ephant::io_error("Expected " + std::to_string(2 * N) + " values, got " + std::to_string(i));
        values[i] = fromString<T>(token);
    }

    for (std::size_t i = 0; i < N; ++i)
        weight.m_table[i] = { values[i], values[N + i] };
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
TaperedWeight<T>::TaperedWeight(std::string name, Tapered<T>& value) 
    : IWeight(std::move(name))
    , m_value(value)
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

template<typename T, std::size_t N>
TaperedTableWeight<T, N>::TaperedTableWeight(std::string name, std::array<Tapered<T>, N>& table)
    : IWeight(std::move(name))
    , m_table(table)
{
    WeightStore::get()->book(this);
}

template<typename T, std::size_t N>
void TaperedTableWeight<T, N>::accept(WeightStore& store, const std::string& newValue)
{
    store.visit(*this, newValue);
}

// ----------------------------------------------------------------------------
// Macros
//
// Weights are compiled in as constexpr values unless the build is configured with ELEPHANT_TUNABLE, then they are
// mutable and booked in the WeightStore so res/weights.ini (and later tuning) can change them. Code reads `name` the
// same way in both. The TUNABLE_ variants are always booked, for the store's own tests.
// ----------------------------------------------------------------------------
#define STRINGIZE(x) #x

#define TUNABLE_WEIGHT(name, type, defaultValue)     inline type name = defaultValue;     inline Weight<type> weight_##name(STRINGIZE(name), name);

#define TUNABLE_TAPERED_WEIGHT(name, type, midgame, endgame)     inline Tapered<type> name{ midgame, endgame };     inline TaperedWeight<type> weight_##name(STRINGIZE(name), name);

// a table of tapered pairs zipped from two published tables, see zipTapered.
#define TUNABLE_TAPERED_TABLE(name, type, midgameTable, endgameTable)     inline auto name = zipTapered<type>(midgameTable, endgameTable);     inline TaperedTableWeight<type, std::size(midgameTable)> weight_##name(STRINGIZE(name), name);

#define TUNABLE_MULTIPLIER(name, defaultValue)     inline i16 name = static_cast<i16>(defaultValue * 1024);     inline MultiplierWeight weight_##name(STRINGIZE(name), name);

#ifdef ELEPHANT_TUNABLE
#define WEIGHT(name, type, defaultValue) TUNABLE_WEIGHT(name, type, defaultValue)
#define TAPERED_WEIGHT(name, type, midgame, endgame) TUNABLE_TAPERED_WEIGHT(name, type, midgame, endgame)
#define TAPERED_TABLE(name, type, midgameTable, endgameTable) TUNABLE_TAPERED_TABLE(name, type, midgameTable, endgameTable)
#define MULTIPLIER(name, defaultValue) TUNABLE_MULTIPLIER(name, defaultValue)
#else
#define WEIGHT(name, type, defaultValue)     inline constexpr type name = defaultValue;
#define TAPERED_WEIGHT(name, type, midgame, endgame)     inline constexpr Tapered<type> name{ midgame, endgame };
#define TAPERED_TABLE(name, type, midgameTable, endgameTable)     inline constexpr auto name = zipTapered<type>(midgameTable, endgameTable);
#define MULTIPLIER(name, defaultValue)     inline constexpr i16 name = static_cast<i16>(defaultValue * 1024);
#endif
