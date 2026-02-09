#pragma once
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"          // config, var_index
#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/detail/node.hpp"     // detail::edge_ptr
#include "freddy/detail/operation/zdd_union.hpp"  // detail::zdd_union
#include "freddy/detail/operation/zdd_inter.hpp"  // detail::zdd_inter
#include "freddy/detail/operation/zdd_diff.hpp"   // detail::zdd_diff
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

    // ---- ZDD-specific public operations ----

    // Complement with explicit universe: universe \ f
    auto complement(zdd const& f, zdd const& universe)
    {
        assert(f.mgr == this);
        assert(universe.mgr == this);
        return zdd{diff_set(universe.f, f.f), this};
    }

  private:
    friend zdd;

    // Terminal nodes: {}  and {{}}
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        node_ptr const leaf0{new detail::node<bool, bool>{false}};  // {} (empty family)
        node_ptr const leaf1{new detail::node<bool, bool>{true}};   // {{}} (family with empty set)

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

    // Complement without universe - throws since ZDD complement requires explicit universe
    auto complement(edge_ptr const& /*f*/) -> edge_ptr override
    {
        throw std::logic_error{
            "ZDD complement requires an explicit universe. "
            "Use zdd_manager::complement(f, universe) instead."
        };
    }

    // Conjunction = set intersection (for ZDD, AND semantics maps to intersection)
    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return inter_set(f, g);
    }

    // Disjunction = set union (for ZDD, OR semantics maps to union)
    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return union_set(f, g);
    }

    // Multiplication = set intersection
    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return inter_set(std::move(f), std::move(g));
    }

    // Addition = set union
    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return union_set(std::move(f), std::move(g));
    }

    // Aggregate: edge weight is unused in ZDD, return node value
    [[nodiscard]] auto agg(bool const& /*w*/, bool const& val) const -> bool override
    {
        return val;
    }

    // Combine edge weights using XOR
    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const -> bool override
    {
        return w1 != w2;
    }

    // Merge node values (OR for boolean)
    [[nodiscard]] auto merge(bool const& v1, bool const& v2) const -> bool override
    {
        return v1 || v2;
    }

    // Regular weight: ZDD does not use complemented edges
    [[nodiscard]] auto regw() const -> bool override
    {
        return false;
    }

    // Normalization methods (not used in ZDD)

    [[nodiscard]] auto norm_weight(edge_ptr const& /*hi*/, edge_ptr const& /*lo*/) const -> bool override
    {
        return false;
    }

    [[nodiscard]] auto norm_is_needed(edge_ptr const& /*hi*/, edge_ptr const& /*lo*/) const -> bool override
    {
        return false;
    }

    auto norm_high(edge_ptr const& hi, bool /*w*/, expansion /*t*/) -> edge_ptr override
    {
        return hi;
    }

    auto norm_low(edge_ptr const& lo, bool /*w*/, expansion /*t*/) -> edge_ptr override
    {
        return lo;
    }

    // Denormalization methods

    [[nodiscard]] auto denorm_high(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);
        assert(!f->is_const());
        return f->ch()->br().hi;
    }

    [[nodiscard]] auto denorm_low(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);
        assert(!f->is_const());
        return f->ch()->br().lo;
    }

    // ZDD reduction rule

    // A ZDD node is reducible if its high child is the zero terminal
    [[nodiscard]] auto reducible(edge_ptr const& hi, edge_ptr const& /*lo*/, expansion /*t*/) const -> bool override
    {
        return hi == constant(0);
    }

    // When reducible, the reduced form is the low child (skip the node)
    [[nodiscard]] auto reduced(edge_ptr const& /*hi*/, edge_ptr const& lo, expansion /*t*/) -> edge_ptr override
    {
        return lo;
    }

    // When variable x is not in function f:
    // - cof0(f, x) = f  (all subsets not containing x = entire f)
    // - cof1(f, x) = {} (no subsets contain x)
    [[nodiscard]] auto expanded(edge_ptr const& f, bool const a, expansion /*t*/) const -> edge_ptr override
    {
        return a ? constant(0) : f;
    }

    // ite(f, g, h) = (f ∩ g) ∪ (f̄ ∩ h)
    // Since we don't have complement, we implement using:
    // ite(f, g, h) = (f ∩ g) ∪ (h \ (f ∩ h))
    // For ZDD semantics: choose g if in f, otherwise choose h
    auto ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr override
    {
        assert(f);
        assert(g);
        assert(h);

        // Terminal cases
        if (f == constant(0))
        {
            return h;
        }
        if (f == constant(1))
        {
            return g;
        }
        if (g == h)
        {
            return g;
        }
        if (g == constant(1) && h == constant(0))
        {
            return f;
        }
        if (g == constant(0) && h == constant(1))
        {
            // This would need complement: return diff_set(one, f)
            // But we don't have universe, so use recursive approach
        }

        // Recursive case using ZDD operations
        // ite(f, g, h) = union(inter(f, g), diff(h, inter(f, h)))
        // Simplified: ite(f, g, h) = union(inter(f, g), diff(h, f))
        // This is an approximation that works for most ZDD use cases
        auto fg = inter_set(f, g);
        auto hf = diff_set(h, f);
        return union_set(fg, hf);
    }

    // Helper: get subsets not containing variable x
    [[nodiscard]] auto subset0(edge_ptr const& S, var_index const x) -> edge_ptr
    {
        return this->cof(S, x, false);
    }

    // Helper: get subsets containing variable x
    [[nodiscard]] auto subset1(edge_ptr const& S, var_index const x) -> edge_ptr
    {
        return this->cof(S, x, true);
    }

    // Set union: P ∪ Q (with caching)
    auto union_set(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        assert(P);
        assert(Q);

        // Base cases
        if (P == constant(0)) return Q;
        if (Q == constant(0)) return P;
        if (P == Q) return P;

        // Both are constants
        if (P->is_const() && Q->is_const())
        {
            return (P == constant(1) || Q == constant(1)) ? constant(1) : constant(0);
        }

        // Check cache
        detail::zdd_union<bool, bool> op{P, Q};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = top_var(P, Q);

        auto R0 = union_set(subset0(P, x), subset0(Q, x));
        auto R1 = union_set(subset1(P, x), subset1(Q, x));

        // Use manager::branch to apply ZDD reduction rule
        op.set_result(manager::branch(x, std::move(R1), std::move(R0)));
        return cache(std::move(op))->get_result();
    }

    // Set intersection: P ∩ Q (with caching)
    auto inter_set(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        assert(P);
        assert(Q);

        // Base cases
        if (P == constant(0) || Q == constant(0)) return constant(0);
        if (P == Q) return P;

        // Both are constants
        if (P->is_const() && Q->is_const())
        {
            return (P == constant(1) && Q == constant(1)) ? constant(1) : constant(0);
        }

        // Check cache
        detail::zdd_inter<bool, bool> op{P, Q};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = top_var(P, Q);

        auto R0 = inter_set(subset0(P, x), subset0(Q, x));
        auto R1 = inter_set(subset1(P, x), subset1(Q, x));

        op.set_result(manager::branch(x, std::move(R1), std::move(R0)));
        return cache(std::move(op))->get_result();
    }

    // Set difference: P \ Q (with caching)
    auto diff_set(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        assert(P);
        assert(Q);

        // Base cases
        if (P == constant(0)) return constant(0);
        if (Q == constant(0)) return P;
        if (P == Q) return constant(0);

        // Both are constants
        if (P->is_const() && Q->is_const())
        {
            return (P == constant(1) && Q == constant(0)) ? constant(1) : constant(0);
        }

        // Check cache
        detail::zdd_diff<bool, bool> op{P, Q};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = top_var(P, Q);

        auto R0 = diff_set(subset0(P, x), subset0(Q, x));
        auto R1 = diff_set(subset1(P, x), subset1(Q, x));

        op.set_result(manager::branch(x, std::move(R1), std::move(R0)));
        return cache(std::move(op))->get_result();
    }

    // Wrapper for zdd::operator-=
    auto diff(edge_ptr const& a, edge_ptr const& b) -> edge_ptr
    {
        return diff_set(a, b);
    }
};

// =====================================================================================================================
// ZDD inline methods
// =====================================================================================================================

inline auto zdd::operator&=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->mul(f, rhs.f);
    return *this;
}

inline auto zdd::operator|=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->plus(f, rhs.f);
    return *this;
}

inline auto zdd::operator-=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->diff(f, rhs.f);
    return *this;
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

}  // namespace freddy