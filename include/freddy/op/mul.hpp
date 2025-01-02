#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <functional>  // std::hash
#include <memory>      // std::shared_ptr
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
class mul : public detail::operation  // multiplication
{
  public:
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    mul(edge_ptr f, edge_ptr g) :
            // for finding a cache result based on factors
            f{std::move(f)},
            g{std::move(g)}
    {
        assert(this->f);
        assert(this->g);
    }

    edge_ptr r;  // product result

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f) * detail::p1 + std::hash<edge_ptr>()(g) * detail::p2;
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<mul const&>(op);

        return (f == other.f && g == other.g) || (f == other.g && g == other.f);
    }

    edge_ptr f;  // 1st factor

    edge_ptr g;  // 2nd factor
};

}  // namespace freddy::op
