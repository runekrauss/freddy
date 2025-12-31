#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                 // config
#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/node.hpp"            // detail::edge_ptr
#include "freddy/detail/operation/mul.hpp"   // detail::mul
#include "freddy/detail/operation/plus.hpp"  // detail::plus
#include "freddy/expansion.hpp"              // expansion::pD

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <bit>          // std::bit_width
#include <cassert>      // assert
#include <cmath>        // std::signbit
#include <cstdint>      // std::int32_t
#include <iostream>     // std::cout
#include <limits>       // std::numeric_limits
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // hash
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

class phdd  // (multiplicative) power hybrid decision diagram
{
  public:
    phdd() = default;  // enable default PHDD construction for compatibility with standard containers

    auto operator-() const;

    auto operator*=(phdd const&) -> phdd&;

    auto operator+=(phdd const&) -> phdd&;

    auto operator-=(phdd const&) -> phdd&;

    auto operator~() const;

    auto operator&=(phdd const&) -> phdd&;

    auto operator|=(phdd const&) -> phdd&;

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

    friend auto operator&(phdd lhs, phdd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    friend auto operator|(phdd lhs, phdd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend auto operator^(phdd lhs, phdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    friend auto operator==(phdd const& lhs, phdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same PHDD manager

        return lhs.f == rhs.f;
    }

    friend auto operator!=(phdd const& lhs, phdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend auto operator<<(std::ostream& os, phdd const& g) -> std::ostream&
    {
        os << "PHDD handle: " << g.f << '\n';
        os << "PHDD manager: " << g.mgr;
        return os;
    }

    [[nodiscard]] auto same_node(phdd const& g) const noexcept
    {
        assert(f);
        assert(mgr == g.mgr);  // PHDD g is valid in any case

        return f->ch() == g.f->ch();
    }

    [[nodiscard]] auto weight() const noexcept
    {
        assert(f);

        return f->weight();
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

        return phdd{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return phdd{f->ch()->br().lo, mgr};
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto is_two() const noexcept;

    template <typename TruthValue, typename... TruthValues>
    auto fn(TruthValue, TruthValues...) const;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto ite(phdd const&, phdd const&) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto has_const(double) const;

    [[nodiscard]] auto is_essential(var_index) const noexcept;

    [[nodiscard]] auto compose(var_index, phdd const&) const;

    [[nodiscard]] auto restr(var_index, bool) const;

    [[nodiscard]] auto exist(var_index) const;

    [[nodiscard]] auto forall(var_index) const;

    [[nodiscard]] auto support() const;

    auto dump_dot(std::ostream& = std::cout) const;

  private:
    friend phdd_manager;

    // wrapper is controlled by its PHDD manager
    phdd(detail::edge_ptr<phdd_weight, double> f, phdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(this->mgr);
    }

    detail::edge_ptr<phdd_weight, double> f;  // PHDD handle

    phdd_manager* mgr{};  // must be destroyed after this PHDD wrapper
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

    [[nodiscard]] auto size(std::vector<phdd> const& fs) const
    {
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<phdd> const& fs) const
    {
        assert(!fs.empty());

        return manager::depth(transform(fs));
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

        manager::dump_dot(transform(fs), outputs, os);
    }

  private:
    friend phdd;

    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        return {edge_ptr{new edge{{false, 0}, new node{0.0}}}, edge_ptr{new edge{{false, 0}, new node{1.0}}}};
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    static auto transform(std::vector<phdd> const& gs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }

    static auto factorize_pow2(std::uint64_t const w) -> std::pair<std::uint64_t, std::uint64_t>
    {
        auto const exp = std::countr_zero(w);
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

    [[nodiscard]] auto normw(edge_ptr const& f, edge_ptr const& g) const noexcept
    {
        if (f == manager::constant(0))
        {
            return g->weight();
        }
        if (g == manager::constant(0))
        {
            return f->weight();
        }
        return phdd_weight{f->weight().first, std::min(f->weight().second, g->weight().second)};
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

        auto const w = normw(hi, lo);
        return uedge(w, unode(x, uedge({hi->weight().first ^ w.first, hi->weight().second - w.second}, hi->ch()),
                              uedge({lo->weight().first ^ w.first, lo->weight().second - w.second}, lo->ch())));
    }

    auto cof(edge_ptr const& f, var_index const x, bool const a) -> edge_ptr override
    {
        assert(f);
        assert(x < var_count());

        if (f->is_const() || f->ch()->br().x != x)
        {
            return decomposition(x) == expansion::pD && a ? manager::constant(0) : f;
        }
        return a ? apply(f->weight(), f->ch()->br().hi) : apply(f->weight(), f->ch()->br().lo);
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
        auto const w = normw(f, g);
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

inline auto phdd::operator~() const
{
    assert(mgr);

    return phdd{mgr->complement(f), mgr};
}

inline auto phdd::operator&=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);

    return *this;
}

inline auto phdd::operator|=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);

    return *this;
}

inline auto phdd::operator^=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);

    return *this;
}

inline auto phdd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

inline auto phdd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

inline auto phdd::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

template <typename TruthValue, typename... TruthValues>
inline auto phdd::fn(TruthValue const a, TruthValues... as) const
{
    assert(mgr);

    return phdd{mgr->fn(f, a, std::forward<TruthValues>(as)...), mgr};
}

inline auto phdd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);

    return mgr->eval(f, as);
}

inline auto phdd::ite(phdd const& g, phdd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return phdd{mgr->ite(f, g.f, h.f), mgr};
}

inline auto phdd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

inline auto phdd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

inline auto phdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

inline auto phdd::has_const(double const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

inline auto phdd::is_essential(var_index const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

inline auto phdd::compose(var_index const x, phdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return phdd{mgr->compose(f, x, g.f), mgr};
}

inline auto phdd::restr(var_index const x, bool const a) const
{
    assert(mgr);

    return phdd{mgr->restr(f, x, a), mgr};
}

inline auto phdd::exist(var_index const x) const
{
    assert(mgr);

    return phdd{mgr->exist(f, x), mgr};
}

inline auto phdd::forall(var_index const x) const
{
    assert(mgr);

    return phdd{mgr->forall(f, x), mgr};
}

inline auto phdd::dump_dot(std::ostream& os) const
{
    assert(mgr);

    mgr->dump_dot({*this}, {}, os);
}

}  // namespace freddy
