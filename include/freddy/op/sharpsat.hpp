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
class sharpsat : public detail::operation  // sharp satisfiability problem
{
  public:
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    explicit sharpsat(edge_ptr f) :
            // for finding a cache result based on #SAT input
            f{std::move(f)}
    {
        assert(this->f);
    }

    double r{};  // #SAT result
  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f);
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<sharpsat const&>(op);

        return f == other.f;
    }

    auto print(std::ostream& s) const -> void override
    {
        s << '(' << f << ")->" << r;
    }

    edge_ptr f;  // #SAT instance
};

}  // namespace freddy::op
