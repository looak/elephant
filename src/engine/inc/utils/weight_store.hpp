#pragma once

#include <unordered_map>
#include <string>

class IWeight {
public:
    IWeight();
    virtual ~IWeight() = 0;

    const std::string& readName() const;

protected:
    std::string m_name;
};

template<typename T>
class TaperedWeight : IWeight {

};

template<typename T>
class Weight : IWeight {
    public:
    Weight(std::string name, T value)
    : m_name(name)
    , m_value(value)
    { }

private:
    T& m_value;
};

class WeightStore {
public:

const WeightStore& get();
void book(IWeight* weight);

void loadFromFile(const std::string& filename);

private:
//template<typename T>
//void updateWeight()

static WeightStore instance;
std::unordered_map<std::string, IWeight*> m_weights;
};

#define WEIGHT(name, type, defaultValue) \
type name = defaultValue;  \
Weight weight_##name<type>("##name", &name); \