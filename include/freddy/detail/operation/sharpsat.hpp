#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"     // hashable
#include "freddy/detail/edge.hpp"       // edge
#include "freddy/detail/node.hpp"       // edge_ptr
#include "freddy/detail/operation.hpp"  // operation

#include <cassert>     // assert
#include <cmath>       // std::isnan
#include <functional>  // std::hash
#include <limits>      // std::numeric_limits

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
class sharpsat final : public operation  // sharp satisfiability problem
{
  public:
    // for looking up a cached result using #SAT input
    explicit sharpsat(edge_ptr<EWeight, NValue> const& f) :
            f{f.get()}
    {
        assert(this->f);
    }

    [[nodiscard]] auto get_result() const noexcept
    {
        assert(!std::isnan(res));

        return res;
    }

    auto set_result(double const res) noexcept
    {
        assert(res >= 0);
        assert(std::isnan(this->res));  // ensure a valid #SAT result is only set once

        this->res = res;
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge<EWeight, NValue>*>{}(f);
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        return f == static_cast<sharpsat const&>(op).f;
    }

    edge<EWeight, NValue>* f;  // #SAT instance

    double res{std::numeric_limits<double>::quiet_NaN()};  // #SAT result, where NaN is the sentinel
};

}  // namespace freddy::detail
