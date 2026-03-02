#pragma once
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                 // config, var_index
#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/node.hpp"            // detail::edge_ptr
#include "freddy/detail/operation/conj.hpp"  // detail::conj
#include "freddy/expansion.hpp"       // expansion::S

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace freddy
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

class zdd_manager;

// =====================================================================================================================
// ZDD wrapper (handle class)
// =====================================================================================================================

class zdd final
{
  public:
    zdd() noexcept = default;

    auto operator&=(zdd const&) -> zdd&;
    auto operator|=(zdd const&) -> zdd&;
    auto operator-=(zdd const&) -> zdd&;

    //negation (Complement)
    auto operator~() const -> zdd;

    friend auto operator&(zdd lhs, zdd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    friend auto operator|(zdd lhs, zdd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend auto operator-(zdd lhs, zdd const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    friend auto operator==(zdd const& lhs, zdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);
        return lhs.f == rhs.f;
    }

    friend auto operator!=(zdd const& lhs, zdd const& rhs) noexcept
    {
        return !(lhs == rhs);
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
        return zdd{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        assert(mgr);
        assert(!is_const());
        return zdd{f->ch()->br().lo, mgr};
    }

    [[nodiscard]] auto is_zero() const noexcept;
    [[nodiscard]] auto is_one() const noexcept;
    [[nodiscard]] auto size() const;
    [[nodiscard]] auto depth() const;

    auto dump_dot(std::ostream& = std::cout) const;

  private:
    friend zdd_manager;

    zdd(detail::edge_ptr<bool, bool> f, zdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(this->mgr);
    }

    detail::edge_ptr<bool, bool> f;
    zdd_manager* mgr{};
};

// =====================================================================================================================
// ZDD manager
// =====================================================================================================================

class zdd_manager final : public detail::manager<bool, bool>
{
  public:
    explicit zdd_manager(struct config const cfg = {}) :
            manager{tmls(), cfg}
    {}

    auto var(std::string_view lbl = {})
    {
        return zdd{manager::var(expansion::S, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        return zdd{manager::var(x), this};
    }

    auto zero() noexcept
    {
        return zdd{constant(0), this};
    }

    auto one() noexcept
    {
        return zdd{constant(1), this};
    }

    [[nodiscard]] auto size(std::vector<zdd> const& fs) const
    {
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<zdd> const& fs) const
    {
        assert(!fs.empty());
        return manager::depth(transform(fs));
    }

    auto dump_dot(std::vector<zdd> const& fs,
                  std::vector<std::string> const& outputs = {},
                  std::ostream& os = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());
        manager::dump_dot(transform(fs), outputs, os);
    }

  private:
    friend zdd;

    // Terminal nodes: {} (empty family) and {{}} (family containing empty set)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        node_ptr const leaf0{new detail::node<bool, bool>{false}};  // {} (empty family) -> 0
        node_ptr const leaf1{new detail::node<bool, bool>{true}};   // {{}} (unit family) -> 1

        return {
            edge_ptr{new detail::edge<bool, bool>{false, leaf0}},  // constant(0)
            edge_ptr{new detail::edge<bool, bool>{false, leaf1}},  // constant(1)
        };
    }

    static auto transform(std::vector<zdd> const& gs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }

    // if variable not present, return f itself ("don't care")
    auto bdd_cof(edge_ptr const& f, var_index const x, bool positive) -> edge_ptr
    {
        if (f->is_const() || f->ch()->br().x != x)
        {
            return f;  // BDD: variable not present -> return f
        }
        return positive ? denorm_high(f) : denorm_low(f);
    }

    // Conjunction copied from BDD
    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == constant(0) || g == constant(0))
        {
            return constant(0);
        }
        if (f == constant(1))
        {
            return g;
        }
        if (g == constant(1))
        {
            return f;
        }
        if (f == g)
        {
            return f;
        }

        detail::conj op{f, g};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = top_var(f, g);

        op.set_result(branch(x, conj(bdd_cof(f, x, true), bdd_cof(g, x, true)),
                                conj(bdd_cof(f, x, false), bdd_cof(g, x, false))));
        return cache(std::move(op))->get_result();
    }


    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return disj_rec(f, g, 0);
    }

    auto disj_rec(edge_ptr const& f, edge_ptr const& g, var_index level) -> edge_ptr
    {
        //  Boolean OR on constants
        if (level >= static_cast<var_index>(var_count()))
        {
            return (f == constant(0) && g == constant(0)) ? constant(0) : constant(1);
        }

        // BDD-style cofactors at current level
        auto f_hi = (!f->is_const() && f->ch()->br().x == level) ? denorm_high(f) : f;
        auto f_lo = (!f->is_const() && f->ch()->br().x == level) ? denorm_low(f) : f;
        auto g_hi = (!g->is_const() && g->ch()->br().x == level) ? denorm_high(g) : g;
        auto g_lo = (!g->is_const() && g->ch()->br().x == level) ? denorm_low(g) : g;

        auto hi = disj_rec(f_hi, g_hi, level + 1);
        auto lo = disj_rec(f_lo, g_lo, level + 1);

        return manager::branch(level, std::move(hi), std::move(lo));
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        return complement_rec(f, 0);
    }

    // Recursive helper
    auto complement_rec(edge_ptr const& f, var_index level) -> edge_ptr
    {
        if (level >= static_cast<var_index>(var_count()))
        {
            return (f == constant(0)) ? constant(1) : constant(0);
        }

        // Cofactors
        auto f0 = subset0(f, level);
        auto f1 = subset1(f, level);

        auto c0 = complement_rec(f0, level + 1);
        auto c1 = complement_rec(f1, level + 1);

        return manager::branch(level, std::move(c1), std::move(c0));
    }

    //  recursive difference function: P \ Q
    // P \ Q = (P0 \ Q0) + x(P1 \ Q1)
    auto diff_recursive(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        if (P == constant(0)) return constant(0);
        if (Q == constant(0)) return P;
        if (P == Q) return constant(0);

        // If P=1 ({{}}), result is 1 if Q doesn't contain {{}}, else 0
        auto const x = top_var(P, Q);

        auto r0 = diff_recursive(subset0(P, x), subset0(Q, x));
        auto r1 = diff_recursive(subset1(P, x), subset1(Q, x));

        return manager::branch(x, std::move(r1), std::move(r0));
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return conj(f, g);
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return disj(f, g);
    }

    // Rest of standard overrides
    [[nodiscard]] auto agg(bool const&, bool const& val) const -> bool override
    {
        return val;
    }
    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const -> bool override
    {
        return w1 != w2;
    }
    [[nodiscard]] auto merge(bool const& v1, bool const& v2) const -> bool override
    {
        return v1 || v2;
    }
    [[nodiscard]] auto regw() const -> bool override
    {
        return false;
    }
    [[nodiscard]] auto norm_weight(edge_ptr const&, edge_ptr const&) const -> bool override
    {
        return false;
    }
    [[nodiscard]] auto norm_is_needed(edge_ptr const&, edge_ptr const&) const -> bool override
    {
        return false;
    }
    auto norm_high(edge_ptr const& hi, bool, expansion) -> edge_ptr override
    {
        return hi;
    }
    auto norm_low(edge_ptr const& lo, bool, expansion) -> edge_ptr override
    {
        return lo;
    }

    [[nodiscard]] auto denorm_high(edge_ptr const& f) -> edge_ptr override
    {
        assert(f); assert(!f->is_const()); return f->ch()->br().hi;
    }
    [[nodiscard]] auto denorm_low(edge_ptr const& f) -> edge_ptr override
    {
        assert(f); assert(!f->is_const()); return f->ch()->br().lo;
    }

    [[nodiscard]] auto reducible(edge_ptr const& hi, edge_ptr const&, expansion) const -> bool override
    {
        return hi == constant(0);
    }
    [[nodiscard]] auto reduced(edge_ptr const&, edge_ptr const& lo, expansion) -> edge_ptr override
    {
        return lo;
    }
    [[nodiscard]] auto expanded(edge_ptr const& f, bool const a, expansion) const -> edge_ptr override
    {
        return a ? constant(0) : f;  // ZDD: hi-cofactor=0 when variable missing, lo-cofactor=f
    }

    // get subsets not containing variable x
    [[nodiscard]] auto subset0(edge_ptr const& S, var_index const x) -> edge_ptr
    {
        return this->cof(S, x, false);
    }

    // get subsets containing variable x
    [[nodiscard]] auto subset1(edge_ptr const& S, var_index const x) -> edge_ptr
    {
        return this->cof(S, x, true);
    }

    // Difference wrapper for operator-=
    auto diff(edge_ptr const& a, edge_ptr const& b) -> edge_ptr
    {
        return diff_recursive(a, b);
    }
};

// =====================================================================================================================
// ZDD inline methods
// =====================================================================================================================

inline auto zdd::operator&=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->conj(f, rhs.f);
    return *this;
}

inline auto zdd::operator|=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->disj(f, rhs.f);
    return *this;
}

inline auto zdd::operator-=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->diff(f, rhs.f);
    return *this;
}

inline auto zdd::operator~() const -> zdd
{
    assert(mgr);
    return zdd{mgr->complement(f), mgr};
}

inline auto zdd::is_zero() const noexcept
{
    assert(mgr);
    return *this == mgr->zero();
}

inline auto zdd::is_one() const noexcept
{
    assert(mgr);
    return *this == mgr->one();
}

inline auto zdd::size() const
{
    assert(mgr);
    return mgr->size({*this});
}

inline auto zdd::depth() const
{
    assert(mgr);
    return mgr->depth({*this});
}

inline auto zdd::dump_dot(std::ostream& os) const
{
    assert(mgr);
    mgr->dump_dot({*this}, {}, os);
}

} // namespace freddy