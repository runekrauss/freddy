#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <functional>  // std::hash
#include <utility>     // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::op
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
class sharpsat : public detail::operation  // sharp satisfiability problem
{
  public:
    // for finding a cache result based on #SAT input
    explicit sharpsat(detail::edge_ptr<E, V> f) :
            f{std::move(f)}
    {
        assert(this->f);
    }

    auto result() noexcept -> double&
    {
        return r;
    }

    [[nodiscard]] auto result() const noexcept -> double const&
    {
        return r;
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<detail::edge_ptr<E, V>>()(f);
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        return f == static_cast<sharpsat const&>(op).f;
    }

    detail::edge_ptr<E, V> f;  // #SAT instance

    double r{};  // #SAT result
};

}  // namespace freddy::op
