#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                 // config
#include "freddy/detail/common.hpp"          // detail::hashable
#include "freddy/detail/dd_base.hpp"         // detail::dd_base
#include "freddy/detail/edge.hpp"            // detail::edge
#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/node.hpp"            // detail::edge_ptr
#include "freddy/detail/operation/mul.hpp"   // detail::mul
#include "freddy/detail/operation/plus.hpp"  // detail::plus
#include "freddy/expansion.hpp"              // expansion::S

#include <boost/algorithm/string.hpp>  // boost::replace_all
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <boost/safe_numerics/safe_integer.hpp>  // boost::safe_numerics::safe
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::isinf
#include <concepts>     // std::floating_point
#include <iostream>     // std::cout
#include <ostream>      // std::ostream
#include <sstream>      // std::ostringstream
#include <stdexcept>    // std::overflow_error
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <type_traits>  // std::is_integral_v
#include <utility>      // std::move
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

template <detail::hashable NValue>
    requires std::floating_point<NValue> || std::integral<NValue>
class add_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

template <detail::hashable NValue>
class add final : public detail::dd_base<add<NValue>, bool, NValue,
                                         add_manager<NValue>>  // algebraic decision diagram (multi-terminal binary
                                                               // decision diagram)
{
    using base = detail::dd_base<add, bool, NValue, add_manager<NValue>>;

    friend add_manager<NValue>;
    friend base;

    // wrapper is controlled by its ADD manager
    add(detail::edge_ptr<bool, NValue> f, add_manager<NValue>* const mgr) :
            base{std::move(f), mgr}
    {}

  public:
    static constexpr std::string_view LABEL = "ADD";

    add() noexcept = default;

    auto operator-() const;

    auto operator*=(add const&) -> add&;

    auto operator+=(add const&) -> add&;

    auto operator-=(add const&) -> add&;

    auto operator^=(add const&) -> add&;

    friend auto operator*(add lhs, add const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    friend auto operator+(add lhs, add const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    friend auto operator-(add lhs, add const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    friend auto operator^(add lhs, add const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    [[nodiscard]] auto is_two() const noexcept;

    [[nodiscard]] auto has_const(NValue) const;
};

template <detail::hashable NValue>  // codomain is a finite set of real numbers or integers
    requires std::floating_point<NValue> || std::integral<NValue>
class add_manager final : public detail::manager<bool, NValue>
{
  public:
    explicit add_manager(struct config const cfg = {}) :
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) because ADD terminals are intrusive
            manager{tmls(), cfg}
    {
        manager::constant(false, 2, true);
        manager::constant(false, -1, true);
    }

    auto var(std::string_view lbl = {})
    {
        return add{manager::var(expansion::S, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        return add{manager::var(x), this};
    }

    auto constant(NValue const val, bool const keep_alive = false)
    {
        assert(!std::isinf(val) && !std::isnan(val));

        return add{manager::constant(false, val, keep_alive), this};
    }

    auto zero() noexcept
    {
        return add{manager::constant(0), this};
    }

    auto one() noexcept
    {
        return add{manager::constant(1), this};
    }

    auto two() noexcept
    {
        return add{manager::constant(2), this};
    }

    [[nodiscard]] auto size(std::vector<add<NValue>> const& fs) const
    {
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<add<NValue>> const& fs) const
    {
        assert(!fs.empty());

        return manager::depth(transform(fs));
    }

    auto dump_dot(std::vector<add<NValue>> const& fs, std::vector<std::string> const& outputs = {},
                  std::ostream& os = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        std::ostringstream oss;
        manager::dump_dot(transform(fs), outputs, oss);

        auto dot = oss.str();
        boost::replace_all(dot, "label=\" 0 \"]", "]");  // as no different edge weights are used
        os << dot;
    }

  private:
    using manager = detail::manager<bool, NValue>;

    using edge_ptr = detail::edge_ptr<bool, NValue>;

    friend add<NValue>;

    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        using edge = detail::edge<bool, NValue>;
        using node = detail::node<bool, NValue>;

        return {edge_ptr{new edge{false, new node{0}}}, edge_ptr{new edge{false, new node{1}}}};
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    static auto transform(std::vector<add<NValue>> const& gs)
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return f == manager::constant(0) ? f : mul(manager::constant(3), f);  // -1f
    }

    auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        return plus(f, neg(g));
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        return sub(plus(f, g), mul(manager::constant(2), mul(f, g)));
    }

    [[nodiscard]] auto agg(bool const& w, NValue const& val) const noexcept -> NValue override
    {
        assert(!w);

        return w + val;
    }

    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const noexcept -> bool override
    {
        assert(!w1);
        assert(!w2);

        return w1 && w2;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        return sub(manager::constant(1), f);
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return mul(f, g);
    }

    auto denorm_high(edge_ptr const& f) -> edge_ptr override
    {
        return this->apply(f->weight(), f->ch()->br().hi);
    }

    auto denorm_low(edge_ptr const& f) -> edge_ptr override
    {
        return this->apply(f->weight(), f->ch()->br().lo);
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == manager::constant(0))
        {
            return g;
        }
        if (g == manager::constant(0))
        {
            return f;
        }
        return sub(plus(f, g), mul(f, g));
    }

    auto expanded(edge_ptr const& f, bool, expansion) const noexcept -> edge_ptr override
    {
        return f;
    }

    [[nodiscard]] auto merge(NValue const& val1, [[maybe_unused]] NValue const& val2) const noexcept -> NValue override
    {
        return val1 * 0;  // as no Davio expansion is used
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == manager::constant(0) || g == manager::constant(0))
        {
            return manager::constant(0);  // f/g * 0 = 0
        }
        if (f == manager::constant(1))
        {
            return g;  // 1 * g = g
        }
        if (g == manager::constant(1))
        {
            return f;  // f * 1 = f
        }

        if (f->is_const() && g->is_const())
        {
            if constexpr (std::is_integral_v<NValue>)  // integer overflow leads to undefined behavior
            {
                boost::safe_numerics::safe<NValue> const fc = f->ch()->value();
                boost::safe_numerics::safe<NValue> const gc = g->ch()->value();
                return manager::constant(false, static_cast<NValue>(fc * gc), false);
            }
            else
            {
                auto const val = f->ch()->value() * g->ch()->value();
                if (std::isinf(val))  // NaN is impossible
                {
                    throw std::overflow_error{"The value \"inf\" resulted after multiplication. "
                                              "Only use constants so that results can be represented."};
                }
                return manager::constant(false, val, false);
            }
        }

        detail::mul op{f, g};
        if (auto const* const entry = this->cached(op))
        {
            return entry->get_result();
        }

        auto const x = this->top_var(f, g);

        op.set_result(this->branch(x, mul(this->cof(f, x, true), this->cof(g, x, true)),
                                   mul(this->cof(f, x, false), this->cof(g, x, false))));
        return this->cache(std::move(op))->get_result();
    }

    auto norm_high(edge_ptr const& hi, bool const w, expansion) -> edge_ptr override
    {
        return this->apply(w, hi);
    }

    auto norm_is_needed(edge_ptr const&, edge_ptr const&) const noexcept -> bool override
    {
        return false;
    }

    auto norm_low(edge_ptr const& lo, bool const w, expansion) -> edge_ptr override
    {
        return this->apply(w, lo);
    }

    auto norm_weight(edge_ptr const&, edge_ptr const& lo) const noexcept -> bool override
    {
        return lo->weight();
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == manager::constant(0))
        {
            return g;  // 0 + g = g
        }
        if (g == manager::constant(0))
        {
            return f;  // f + 0 = f
        }

        if (f->is_const() && g->is_const())
        {
            if constexpr (std::is_integral_v<NValue>)
            {
                boost::safe_numerics::safe<NValue> const fc = f->ch()->value();
                boost::safe_numerics::safe<NValue> const gc = g->ch()->value();
                return manager::constant(false, static_cast<NValue>(fc + gc), false);
            }
            else
            {
                auto const val = f->ch()->value() + g->ch()->value();
                if (std::isinf(val))
                {
                    throw std::overflow_error{"The value \"inf\" resulted after addition. "
                                              "Only use constants so that results can be represented."};
                }
                return manager::constant(false, val, false);
            }
        }

        detail::plus op{f, g};
        if (auto const* const entry = this->cached(op))
        {
            return entry->get_result();
        }

        auto const x = this->top_var(f, g);

        op.set_result(this->branch(x, plus(this->cof(f, x, true), this->cof(g, x, true)),
                                   plus(this->cof(f, x, false), this->cof(g, x, false))));
        return this->cache(std::move(op))->get_result();
    }

    auto reduced(edge_ptr const& hi, edge_ptr const&, expansion) noexcept -> edge_ptr override
    {
        return hi;
    }

    auto reducible(edge_ptr const& hi, edge_ptr const& lo, expansion) const noexcept -> bool override
    {
        return hi == lo;
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;
    }
};

template <detail::hashable NValue>
inline auto add<NValue>::operator-() const
{
    assert(this->mgr);

    return add{this->mgr->neg(this->f), this->mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::operator*=(add const& rhs) -> add&
{
    assert(this->mgr);
    assert(this->mgr == rhs.mgr);

    this->f = this->mgr->mul(this->f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator+=(add const& rhs) -> add&
{
    assert(this->mgr);
    assert(this->mgr == rhs.mgr);

    this->f = this->mgr->plus(this->f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator-=(add const& rhs) -> add&
{
    assert(this->mgr);
    assert(this->mgr == rhs.mgr);

    this->f = this->mgr->sub(this->f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator^=(add const& rhs) -> add&
{
    assert(this->mgr);
    assert(this->mgr == rhs.mgr);

    this->f = this->mgr->antiv(this->f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::is_two() const noexcept
{
    assert(this->mgr);

    return *this == this->mgr->two();
}

template <detail::hashable NValue>
inline auto add<NValue>::has_const(NValue const c) const
{
    assert(this->mgr);

    return this->mgr->has_const(this->f, c);
}

}  // namespace freddy
