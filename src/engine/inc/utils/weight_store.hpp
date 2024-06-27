#pragma once

#include <fstream>
#include <unordered_map>
#include <sstream>
#include <string>

#include "defines.hpp"

// Abstract file reader interface
class FileReader {
public:
    virtual ~FileReader() = default;
    virtual bool openFile(const std::string& filename) = 0;
    virtual bool readLine(std::string& line) = 0;
};

// Concrete implementation for real file reading
class RealFileReader : public FileReader {
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

    virtual void accept(WeightStore& store, const std::string& newValue) override {
        std::stringstream ss(newValue);
        std::string a, b;
        ss >> a >> b;
        store.visit(*this, a, b);
    }

    i32 operator*(const float& t) {
        return m_a + (m_b - m_a) * t;
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

    virtual void accept(WeightStore & store, const std::string & newValue) override {
        store.visit(*this, newValue);
    }

private:
    friend class WeightStore;
    T& m_value;
};

class WeightStore {
public:
    void initialize(FileReader* fileReader = new RealFileReader());
    static WeightStore* get();

    void book(IWeight* weight);

    void loadFromFile(const std::string& filename);
    void update(const std::string& name, const std::string& newValue);

    void visit(Weight<i64>& weight, const std::string& newValueStr);
    void visit(Weight<i32>& weight, const std::string& newValueStr);
    void visit(Weight<double>& weight, const std::string& newValueStr);
    void visit(Weight<bool>& weight, const std::string& newValueStr);
    void visit(Weight<std::string>& weight, const std::string& newValueStr);

    void visit(TaperedWeight<i64>& weight, const std::string& a, const std::string& b);
    void visit(TaperedWeight<i32>& weight, const std::string& a, const std::string& b);
    void visit(TaperedWeight<double>& weight, const std::string& a, const std::string& b);

private:
    static WeightStore* instance;
    WeightStore() = default;

    FileReader* m_fileReader;
    std::unordered_map<std::string, IWeight*> m_weights;
};

#define STRINGIZE(x) #x
#define WEIGHT(name, type, defaultValue) \
type name = defaultValue;  \
Weight<type> weight_##name(STRINGIZE(name), name);

#define TAPERED_WEIGHT(name, type, a, b) \
type name##_a = a; \
type name##_b = b; \
TaperedWeight<type> name(STRINGIZE(name), name##_a, name##_b);