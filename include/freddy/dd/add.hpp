#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                 // config
#include "freddy/detail/common.hpp"          // detail::hashable
#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/node.hpp"            // detail::edge_ptr
#include "freddy/detail/operation/mul.hpp"   // detail::mul
#include "freddy/detail/operation/plus.hpp"  // detail::plus
#include "freddy/expansion.hpp"              // expansion::S

#include <boost/algorithm/string.hpp>            // boost::replace_all
#include <boost/safe_numerics/safe_integer.hpp>  // boost::safe_numerics::safe

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
class add final  // algebraic decision diagram (multi-terminal binary decision diagram)
{
  public:
    add() noexcept = default;  // enable default ADD construction for compatibility with standard containers

    auto operator-() const;

    auto operator*=(add const&) -> add&;

    auto operator+=(add const&) -> add&;

    auto operator-=(add const&) -> add&;

    auto operator~() const;

    auto operator&=(add const&) -> add&;

    auto operator|=(add const&) -> add&;

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

    friend auto operator&(add lhs, add const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    friend auto operator|(add lhs, add const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend auto operator^(add lhs, add const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    friend auto operator==(add const& lhs, add const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same ADD manager

        return lhs.f == rhs.f;
    }

    friend auto operator!=(add const& lhs, add const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend auto operator<<(std::ostream& os, add const& g) -> std::ostream&
    {
        os << "ADD handle: " << g.f << '\n';
        os << "ADD manager: " << g.mgr;
        return os;
    }

    [[nodiscard]] auto same_node(add const& g) const noexcept
    {
        assert(f);
        assert(mgr == g.mgr);  // ADD g is valid in any case

        return f->ch() == g.f->ch();
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        assert(f);

        return f->is_const();
    }

    [[nodiscard]] auto var() const noexcept
    {
        assert(!is_const());

        return f->ch()->br().x;
    }

    [[nodiscard]] auto high() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return add{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return add{f->ch()->br().lo, mgr};
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto is_two() const noexcept;

    template <typename TruthValue, typename... TruthValues>
    [[nodiscard]] auto fn(TruthValue, TruthValues...) const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto ite(add const&, add const&) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto has_const(NValue) const;

    [[nodiscard]] auto is_essential(var_index) const noexcept;

    [[nodiscard]] auto compose(var_index, add const&) const;

    [[nodiscard]] auto restr(var_index, bool) const;

    [[nodiscard]] auto exist(var_index) const;

    [[nodiscard]] auto forall(var_index) const;

    auto dump_dot(std::ostream& = std::cout) const;

  private:
    friend add_manager<NValue>;

    // wrapper is controlled by its ADD manager
    add(detail::edge_ptr<bool, NValue> f, add_manager<NValue>* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(this->mgr);
    }

    detail::edge_ptr<bool, NValue> f;  // ADD handle

    add_manager<NValue>* mgr{};  // must be destroyed after this ADD wrapper
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

    [[nodiscard]] auto agg([[maybe_unused]] bool const& w, NValue const& val) const noexcept -> NValue override
    {
        return val;
    }

    auto branch(var_index const x, edge_ptr&& hi, edge_ptr&& lo) -> edge_ptr override
    {
        assert(x < this->var_count());
        assert(hi);
        assert(lo);

        return hi == lo ? hi : this->uedge(false, this->unode(x, std::move(hi), std::move(lo)));
    }

    [[nodiscard]] auto comb([[maybe_unused]] bool const& w1, [[maybe_unused]] bool const& w2) const noexcept
        -> bool override
    {
        return false;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        return sub(manager::constant(1), f);
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return mul(f, g);
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

    [[nodiscard]] auto merge([[maybe_unused]] NValue const& val1, [[maybe_unused]] NValue const& val2) const noexcept
        -> NValue override
    {
        return 0;  // as no Davio expansion is used
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

        op.set_result(branch(x, mul(this->cof(f, x, true), this->cof(g, x, true)),
                             mul(this->cof(f, x, false), this->cof(g, x, false))));
        return this->cache(std::move(op))->get_result();
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

        op.set_result(branch(x, plus(this->cof(f, x, true), this->cof(g, x, true)),
                             plus(this->cof(f, x, false), this->cof(g, x, false))));
        return this->cache(std::move(op))->get_result();
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;
    }
};

template <detail::hashable NValue>
inline auto add<NValue>::operator-() const
{
    assert(mgr);

    return add{mgr->neg(f), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::operator*=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator+=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->plus(f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator-=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator~() const
{
    assert(mgr);

    return add{mgr->complement(f), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::operator&=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator|=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::operator^=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);

    return *this;
}

template <detail::hashable NValue>
inline auto add<NValue>::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

template <detail::hashable NValue>
inline auto add<NValue>::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

template <detail::hashable NValue>
inline auto add<NValue>::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

template <detail::hashable NValue>
template <typename TruthValue, typename... TruthValues>
inline auto add<NValue>::fn(TruthValue const a, TruthValues... as) const noexcept  // as no new ADD will be computed
{
    assert(mgr);

    return add{mgr->fn(f, a, std::forward<TruthValues>(as)...), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);

    return mgr->eval(f, as);
}

template <detail::hashable NValue>
inline auto add<NValue>::ite(add const& g, add const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return add{mgr->ite(f, g.f, h.f), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

template <detail::hashable NValue>
inline auto add<NValue>::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

template <detail::hashable NValue>
inline auto add<NValue>::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

template <detail::hashable NValue>
inline auto add<NValue>::has_const(NValue const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

template <detail::hashable NValue>
inline auto add<NValue>::is_essential(var_index const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

template <detail::hashable NValue>
inline auto add<NValue>::compose(var_index const x, add const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return add{mgr->compose(f, x, g.f), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::restr(var_index const x, bool const a) const
{
    assert(mgr);

    return add{mgr->restr(f, x, a), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::exist(var_index const x) const
{
    assert(mgr);

    return add{mgr->exist(f, x), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::forall(var_index const x) const
{
    assert(mgr);

    return add{mgr->forall(f, x), mgr};
}

template <detail::hashable NValue>
inline auto add<NValue>::dump_dot(std::ostream& os) const
{
    assert(mgr);

    mgr->dump_dot({*this}, {}, os);
}

}  // namespace freddy
