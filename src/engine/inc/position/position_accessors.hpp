#pragma once
#include <position/position_proxy.hpp>
#include <position/position_access_policies.hpp>

typedef PositionProxy<PositionEditPolicy> PositionEditor;
typedef PositionProxy<PositionReadOnlyPolicy> PositionReader;