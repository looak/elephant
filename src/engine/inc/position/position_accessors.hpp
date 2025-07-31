#include <position/position_proxy.hpp>

struct PositionEditPolicy {
    using position_t = Position&;
};

struct PositionReadOnlyPolicy {
    using position_t = const Position&;
};

typedef PositionProxy<PositionEditPolicy> PositionEditor;
typedef PositionProxy<PositionReadOnlyPolicy> PositionReader;