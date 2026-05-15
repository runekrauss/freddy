#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                 // config
#include "freddy/detail/dd_base.hpp"         // detail::dd_base
#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/node.hpp"            // detail::edge_ptr
#include "freddy/detail/operation/mul.hpp"   // detail::mul
#include "freddy/detail/operation/plus.hpp"  // detail::plus
#include "freddy/expansion.hpp"              // expansion::pD

#include <array>        // std::array
#include <bit>          // std::bit_width
#include <cassert>      // assert
#include <cmath>        // std::signbit
#include <iostream>     // std::cout
#include <limits>       // std::numeric_limits
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::pair
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace std
{

// hash specialization for multiplicative edge weights
template <typename W1, typename W2>
struct hash<std::pair<W1, W2>> final
{
    auto operator()(std::pair<W1, W2> const& w) const noexcept
    {
        return hash<W2>{}(w.second) ^ (static_cast<unsigned>(w.first) << 31u);
    }
};

template <typename W1, typename W2>
inline auto operator<<(std::ostream& os, std::pair<W1, W2> const& w) -> std::ostream&
{
    if (w.first)
    {
        os << "neg\n";
    }
    return os << w.second;
}

template <typename W1, typename W2>
inline auto operator!([[maybe_unused]] std::pair<W1, W2> const& w) -> std::pair<W1, W2>
{
    return {true, 0};
}

}  // namespace std

namespace freddy
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

class phdd_manager;

// =====================================================================================================================
// Aliases
// =====================================================================================================================

using phdd_weight = std::pair<bool, std::int32_t>;

// =====================================================================================================================
// Types
// =====================================================================================================================

class phdd : public detail::dd_base<phdd, phdd_weight, double,
                                    phdd_manager>  // (multiplicative) power hybrid decision diagram
{
    friend phdd_manager;
    friend dd_base;

    // wrapper is controlled by its PHDD manager
    phdd(detail::edge_ptr<phdd_weight, double> f, phdd_manager* const mgr) :
            dd_base{std::move(f), mgr}
    {}

  public:
    static constexpr std::string_view LABEL = "PHDD";

    phdd() noexcept = default;

    auto operator-() const;

    auto operator*=(phdd const&) -> phdd&;

    auto operator+=(phdd const&) -> phdd&;

    auto operator-=(phdd const&) -> phdd&;

    auto operator^=(phdd const&) -> phdd&;

    friend auto operator*(phdd lhs, phdd const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    friend auto operator+(phdd lhs, phdd const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    friend auto operator-(phdd lhs, phdd const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    friend auto operator^(phdd lhs, phdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    [[nodiscard]] auto weight() const noexcept
    {
        assert(f);

        return f->weight();
    }

    [[nodiscard]] auto is_two() const noexcept;

    [[nodiscard]] auto has_const(double) const;

    [[nodiscard]] auto support() const;

  private:
};

class phdd_manager final : public detail::manager<phdd_weight, double>
{
  public:
    explicit phdd_manager(struct config const cfg = {}) :
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) because PHDD terminals are intrusive
            manager{tmls(), cfg}
    {
        manager::constant({false, 1}, 1.0, true);  // two
        manager::constant({true, 0}, 1.0, true);   // negative one
    }

    auto var(expansion const t, std::string_view lbl = {})
    {
        return phdd{manager::var(t, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        return phdd{manager::var(x), this};
    }

    auto zero() noexcept
    {
        return phdd{manager::constant(0), this};
    }

    auto one() noexcept
    {
        return phdd{manager::constant(1), this};
    }

    auto two() noexcept
    {
        return phdd{manager::constant(2), this};
    }

    auto constant(double const w, bool const keep_alive = false)
    {
        if (w == 0)
        {
            return zero();
        }

        auto d = decompose_double(w);
        if (std::bit_width(std::get<2>(d)) >= 53)
        {
            throw std::invalid_argument("double value escaped supported range");
        }
        return phdd{manager::constant({std::get<0>(d), static_cast<std::int32_t>(std::get<1>(d))},
                                      static_cast<double>(std::get<2>(d)), keep_alive),
                    this};
    }

    auto weighted_sum(std::vector<phdd> const& fs)
    {
        auto res = manager::constant(0);
        for (auto i = 0uz; i < fs.size(); ++i)
        {  // LSB...MSB
            res = plus(res, mul(manager::constant({false, static_cast<std::int32_t>(i)}, 1.0, false), fs[i].f));
        }
        return phdd{res, this};
    }

    auto dump_dot(std::vector<phdd> const& fs, std::vector<std::string> const& outputs = {},
                  std::ostream& os = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        manager::dump_dot(phdd::transform(fs), outputs, os);
    }

  private:
    friend phdd;

    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        return {edge_ptr{new edge{{false, 0}, new node{0.0}}}, edge_ptr{new edge{{false, 0}, new node{1.0}}}};
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    static auto factorize_pow2(std::uint64_t const w) -> std::pair<std::uint64_t, std::uint64_t>
    {
        auto const exp = static_cast<std::uint64_t>(std::countr_zero(w));
        return {exp, w >> exp};
    }

    static auto decompose_double(double x) -> std::tuple<bool, std::uint64_t, std::uint64_t>
    {
        if (std::isnan(x) || std::isinf(x) || !std::isnormal(x))
        {
            throw std::invalid_argument("double value escaped supported range");
        }
        if (x == 0)
        {
            return {false, 0, 0};
        }

        auto const sign = std::signbit(x);
        auto const bits = std::bit_cast<std::uint64_t>(x);
        auto const exponent =
            static_cast<std::int64_t>((bits >> static_cast<std::uint64_t>(52)) & static_cast<std::uint64_t>(0x7FF)) -
            1023 - 52;  // bias 1023, sig_size 52
        auto const significant = (bits & static_cast<std::uint64_t>(0xFFFFFFFFFFFFF)) |
                                 (static_cast<std::uint64_t>(1) << static_cast<std::uint64_t>(52));  // leading zero
        auto const factors = factorize_pow2(significant);
        return std::tuple{sign, exponent + factors.first, factors.second};
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return f == manager::constant(0) ? f : mul(manager::constant(3), f);
    }

    auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return plus(f, neg(g));
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return sub(plus(f, g), mul(manager::constant(2), mul(f, g)));
    }

    [[nodiscard]] auto agg(phdd_weight const& w, double const& val) const noexcept -> double override
    {
        return w.first ? -1 * std::pow(2, w.second) * val : std::pow(2, w.second) * val;
    }

    auto apply(phdd_weight const& w, edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return f == manager::constant(0) || (!w.first && w.second == 0) ? f : uedge(comb(w, f->weight()), f->ch());
    }

    auto branch(var_index const x, edge_ptr&& hi, edge_ptr&& lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (decomposition(x) == expansion::S && hi == lo)
        {
            return hi;
        }
        if (decomposition(x) == expansion::S && hi == manager::constant(0))
        {
            return uedge(lo->weight(), unode(x, std::move(hi), uedge({false, 0}, lo->ch())));
        }
        if (decomposition(x) == expansion::pD && hi == manager::constant(0))
        {
            return lo;
        }
        if (lo == manager::constant(0))
        {
            return uedge(hi->weight(), unode(x, uedge({false, 0}, hi->ch()), std::move(lo)));
        }

        auto const w = norm_weight(hi, lo);
        return uedge(w, unode(x, uedge({hi->weight().first ^ w.first, hi->weight().second - w.second}, hi->ch()),
                              uedge({lo->weight().first ^ w.first, lo->weight().second - w.second}, lo->ch())));
    }

    [[nodiscard]] auto comb(phdd_weight const& w1, phdd_weight const& w2) const noexcept -> phdd_weight override
    {
        return {w1.first ^ w2.first, w1.second + w2.second};
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return sub(manager::constant(1), f);
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return mul(f, g);
    }

    auto denorm_high(edge_ptr const& f) -> edge_ptr override
    {
        return apply(f->weight(), f->ch()->br().hi);
    }

    auto denorm_low(edge_ptr const& f) -> edge_ptr override
    {
        return apply(f->weight(), f->ch()->br().lo);
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

    [[nodiscard]] auto expanded(edge_ptr const& f, bool const a, expansion const t) const noexcept -> edge_ptr override
    {
        return t == expansion::pD && a ? manager::constant(0) : f;
    }

    [[nodiscard]] auto merge(double const& val1, double const& val2) const -> double override
    {
        return val1 + val2;
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == manager::constant(0) || g == manager::constant(0))
        {
            return manager::constant(0);
        }
        if (f->ch() == manager::constant(1)->ch())
        {
            return apply(f->weight(), g);
        }
        if (g->ch() == manager::constant(1)->ch())
        {
            return apply(g->weight(), f);
        }
        if (f->is_const() && g->is_const())
        {
            // check if mul of const node values is exact
            if (std::fma(f->ch()->value(), g->ch()->value(), -(f->ch()->value() * g->ch()->value())) != 0.0)
            {
                throw std::invalid_argument("too big constants, multiplication of constants leads to underflow");
            }
            return manager::constant({f->weight().first ^ g->weight().first, f->weight().second + g->weight().second},
                                     f->ch()->value() * g->ch()->value(), false);
        }

        auto const w = phdd_weight{f->weight().first ^ g->weight().first, f->weight().second + g->weight().second};
        if ((*f->ch())() <= (*g->ch())())
        {
            std::swap(f, g);
        }
        f = uedge({false, 0}, f->ch());
        g = uedge({false, 0}, g->ch());

        detail::mul op{f, g};
        if (auto const* const entry = cached(op))
        {
            return apply(w, entry->get_result());
        }

        auto const x = top_var(f, g);
        edge_ptr res;
        if (decomposition(x) == expansion::S)
        {
            res = branch(x, mul(cof(f, x, true), cof(g, x, true)), mul(cof(f, x, false), cof(g, x, false)));
        }
        else if (decomposition(x) == expansion::pD)
        {
            res = branch(x,
                         plus(plus(mul(cof(f, x, true), cof(g, x, true)), mul(cof(f, x, true), cof(g, x, false))),
                              mul(cof(f, x, false), cof(g, x, true))),
                         mul(cof(f, x, false), cof(g, x, false)));
        }

        op.set_result(res);
        cache(std::move(op));

        return apply(w, res);
    }

    [[nodiscard]] auto norm_high(edge_ptr const&, phdd_weight const, expansion) noexcept -> edge_ptr override
    {
        return manager::constant(0);
    }

    [[nodiscard]] auto norm_is_needed(edge_ptr const&, edge_ptr const&) const noexcept -> bool override
    {
        return true;
    }

    [[nodiscard]] auto norm_low(edge_ptr const&, phdd_weight const, expansion) noexcept -> edge_ptr override
    {
        return manager::constant(0);
    }

    [[nodiscard]] auto norm_weight(edge_ptr const& hi, edge_ptr const& lo) const noexcept -> phdd_weight override
    {
        if (hi == manager::constant(0))
        {
            return lo->weight();
        }
        if (lo == manager::constant(0))
        {
            return hi->weight();
        }
        return {hi->weight().first, std::min(hi->weight().second, lo->weight().second)};
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
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
        if (f->ch() == g->ch() && f->weight().first != g->weight().first && f->weight().second == g->weight().second)
        {
            return manager::constant(0);
        }
        if (f->is_const() && g->is_const())
        {
            if (f->weight().second > g->weight().second)
            {
                std::swap(f, g);
            }  // 2^f_w * (f_vc + 2^(g_w - f_w) * g_vc)
            auto f_vc = static_cast<std::uint64_t>(f->ch()->value());
            auto g_vc = static_cast<std::uint64_t>(g->ch()->value());
            auto shift = static_cast<std::uint64_t>(g->weight().second - f->weight().second);
            auto sign = f->weight().first;
            if (std::cmp_less(std::countl_zero(g_vc), shift))
            {
                throw std::invalid_argument("too big constants, addition of constants leads to underflow");
            }
            g_vc <<= shift;
            std::pair<std::uint64_t, std::uint64_t> factors;
            if (f->weight().first == g->weight().first)
            {
                if (f_vc > std::numeric_limits<std::uint64_t>::max() - g_vc)
                {
                    throw std::invalid_argument("too big constants, addition of constants leads to underflow");
                }
                factors = factorize_pow2(f_vc + g_vc);
            }
            else
            {
                if (f_vc < g_vc)
                {
                    std::swap(f_vc, g_vc);
                    sign = g->weight().first;
                }
                factors = factorize_pow2(f_vc - g_vc);
            }
            if (std::bit_width(factors.second) >= 53)
            {
                throw std::invalid_argument("too big constants, addition of constants leads to underflow");
            }
            return manager::constant({sign, static_cast<std::int32_t>(factors.first + f->weight().second)},
                                     static_cast<double>(factors.second), false);
        }

        if (std::abs(f->weight().second) <= std::abs(g->weight().second))
        {
            std::swap(f, g);
        }
        auto const w = norm_weight(f, g);
        f = uedge({f->weight().first ^ w.first, f->weight().second - w.second}, f->ch());
        g = uedge({g->weight().first ^ w.first, g->weight().second - w.second}, g->ch());

        detail::plus op{f, g};
        if (auto const* const entry = cached(op))
        {
            return apply(w, entry->get_result());
        }

        auto const x = top_var(f, g);
        auto const res = branch(x, plus(cof(f, x, true), cof(g, x, true)), plus(cof(f, x, false), cof(g, x, false)));

        op.set_result(res);
        cache(std::move(op));

        return apply(w, res);
    }

    [[nodiscard]] auto reduced(edge_ptr const&, edge_ptr const&, expansion) noexcept -> edge_ptr override
    {
        return manager::constant(0);
    }

    [[nodiscard]] auto reducible(edge_ptr const&, edge_ptr const&, expansion) const noexcept -> bool override
    {
        return false;
    }

    [[nodiscard]] auto regw() const noexcept -> phdd_weight override
    {
        return {false, 0};
    }
};

inline auto phdd::operator-() const
{
    assert(mgr);

    return phdd{mgr->neg(f), mgr};
}

inline auto phdd::operator*=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);

    return *this;
}

inline auto phdd::operator+=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->plus(f, rhs.f);

    return *this;
}

inline auto phdd::operator-=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);

    return *this;
}

inline auto phdd::operator^=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);

    return *this;
}

inline auto phdd::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

inline auto phdd::has_const(double const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

}  // namespace freddy
