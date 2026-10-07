#pragma once

template<typename T>
class TaperedWeight : IWeight {
public:
    TaperedWeight(std::string name, T& a, T& b) :
        IWeight(name),
        m_a(a),
        m_b(b)
    {
        WeightStore::get()->book(this);
    }

    virtual void accept(WeightStore& store, const std::string& newValue) override;

    i32 operator*(const float& t) { return static_cast<i32>(m_a + (m_b - m_a) * t); }

private:
    friend class WeightStore;
    T& m_a;
    T& m_b;
};

template<typename T>
class Weight : IWeight {
public:
    Weight(std::string name, T& value) :
        IWeight(name),
        m_value(value)
    {
        WeightStore::get()->book(this);
    }

    virtual void accept(WeightStore& store, const std::string& newValue) override;

private:
    friend class WeightStore;
    T& m_value;
};