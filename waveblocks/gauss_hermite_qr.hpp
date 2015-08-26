#pragma once

#include <vector>

#include "basic_types.hpp"
#include "tables_gausshermite.hpp"

namespace waveblocks {

template <dim_t ORDER>
struct GaussHermiteQR
{
    static const dim_t order = ORDER;

    static const std::vector<real_t>& nodes()
    {
        return gauss_hermite_rules[ORDER-1].nodes;
    }

    static const std::vector<real_t>& weights()
    {
        return gauss_hermite_rules[ORDER-1].weights;
    }
};

}