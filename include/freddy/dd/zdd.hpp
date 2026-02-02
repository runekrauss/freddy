#pragma once
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"          // config, var_index
#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/detail/node.hpp"     // detail::edge_ptr
#include "freddy/expansion.hpp"       // expansion::S
#include <stdexcept>

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
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
    zdd() noexcept = default; // copied from bdd.hpp

    auto operator&=(zdd const&) -> zdd&; // copied structure from bdd.hpp
    auto operator|=(zdd const&) -> zdd&; // copied structure from bdd.hpp
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
        // copied from bdd.hpp
        assert(lhs.mgr == rhs.mgr);
        return lhs.f == rhs.f;
    }

    friend auto operator!=(zdd const& lhs, zdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        // copied from bdd.hpp
        assert(f);
        return f->is_const();
    }

    [[nodiscard]] auto var() const noexcept
    {
        // copied from bdd.hpp
        assert(!is_const());
        return f->ch()->br().x;
    }

    [[nodiscard]] auto high() const noexcept
    {
        // copied from bdd.hpp
        assert(mgr);
        assert(!is_const());
        return zdd{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        // copied from bdd.hpp
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

    // copied from bdd.hpp
    zdd(detail::edge_ptr<bool, bool> f, zdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(this->mgr);
    }

    detail::edge_ptr<bool, bool> f; // copied from bdd.hpp
    zdd_manager* mgr{};             // copied from bdd.hpp
};

// =====================================================================================================================
// ZDD manager
// =====================================================================================================================

class zdd_manager final : public detail::manager<bool, bool>
{
  public:
    explicit zdd_manager(struct config const cfg = {}) :
            // copied from bdd.hpp
            manager{tmls(), cfg}
    {}

    auto var(std::string_view lbl = {})
    {
        // copied from bdd.hpp
        return zdd{manager::var(expansion::S, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        // copied from bdd.hpp
        return zdd{manager::var(x), this};
    }

    auto zero() noexcept
    {
        // copied  from bdd.hpp
        return zdd{constant(0), this};
    }

    auto one() noexcept
    {
        // copied  from bdd.hpp
        return zdd{constant(1), this};
    }

    // Public wrappers (because mul/plus are protected in detail::manager)
    auto apply_and(edge_ptr a, edge_ptr b) -> edge_ptr { return mul(std::move(a), std::move(b)); }
    auto apply_or(edge_ptr a, edge_ptr b) -> edge_ptr { return plus(std::move(a), std::move(b)); }
    auto apply_diff(edge_ptr const& a, edge_ptr const& b) -> edge_ptr { return diff_set(a, b); }


    [[nodiscard]] auto size(std::vector<zdd> const& fs) const
    {
        // copied from bdd.hpp
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<zdd> const& fs) const
    {
        // copied from bdd.hpp
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

    // Optional helpers for examples/tests
    auto singleton(var_index x) -> zdd
    {
        return zdd{branch(x, constant(1), constant(0)), this};
    }

    auto node(var_index x, zdd const& hi, zdd const& lo) -> zdd
    {
        assert(hi.mgr == this && lo.mgr == this);
        auto hi_e = hi.f;
        auto lo_e = lo.f;
        return zdd{branch(x, std::move(hi_e), std::move(lo_e)), this};
    }


  private:
    friend zdd;

    // terminal without complemented edges.
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        node_ptr const leaf0{new detail::node<bool, bool>{false}}; // ZDD terminal 0: {}
        node_ptr const leaf1{new detail::node<bool, bool>{true}}; // ZDD terminal 1: {{}}

        return{
            edge_ptr{new detail::edge<bool, bool>{false, leaf0}}, // constant(0)
            edge_ptr{new detail::edge<bool, bool>{false, leaf1}}, // constant(1)
        };
    }

    // copied from bdd.hpp
    static auto transform(std::vector<zdd> const& gs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }


    // Required virtuals for detail::manager<bool,bool>
    auto complement(edge_ptr const& /*f*/) -> edge_ptr override
    {
        throw std::logic_error{"ZDD complement is not supported in this implementation."};
    }

    // and = set intersection
    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return inter_set(f, g);
    }

    // or = set disjunction
    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return union_set(f, g);
    }

    // and
    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return inter_set(f, g);
    }

    // or
    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return union_set(f, g);
    }

    //edge weight will be ignored
    // 0 -> empty {}
    // 1 -> empty set {{}}
    [[nodiscard]] auto agg(bool const& /*w*/, bool const& val) const -> bool override
    {
        //just return the terminal value
        return val;
    }

    // wrote it bcs of manager
    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const -> bool override
    {
        return w1 != w2;
    }

    // wrote it bcs of manager
    [[nodiscard]] auto merge(bool const& v1, bool const& v2) const -> bool override
    {
        return v1 || v2;
    }

    // regular weight = false (no complemented edges)
    [[nodiscard]] auto regw() const -> bool override
    {
        return false;
    }

    // ZDD-specific cofactor:
    // cof0(f,x): all sets not containing x (if x not present at top -> f)
    // cof1(f,x): all sets containing x (if x not present at top -> {})
    auto cof(edge_ptr const& f, var_index const x, bool const a) -> edge_ptr override
    {
        assert(f);
        assert(x < var_count());

        // terminals: do not contain x
        if (f->is_const())
        {
            // cof0(const) = const, cof1(const) = {}
            return a ? constant(0) : f;
        }

        // use public accessors (no direct f->v / ->inner access)
        auto const& br = f->ch()->br(); // branch (x, hi, lo)

        // if top var != x, x does not occur at the top
        if (br.x != x)
        {
            return a ? constant(0) : f;
        }

        // top var is x: take corresponding branch
        return a ? br.hi : br.lo;
    }



    [[nodiscard]] auto subset0(edge_ptr const& S, var_index x) -> edge_ptr
    {
        return cof(S, x, false);
    }

    [[nodiscard]] auto subset1(edge_ptr const& S, var_index x) -> edge_ptr
    {
        return cof(S, x, true);
    }

    // {{}}
    [[nodiscard]] auto epsilon() const noexcept -> edge_ptr
    {
        return constant(1);
    }

    // // {}
    // [[nodiscard]] auto empty() const noexcept -> edge_ptr
    // {
    //     return constant(0);
    // }
    //
    // // {{x}} = node x with hi = {{}} and lo = {}
    // [[nodiscard]] auto single(var_index x) -> edge_ptr
    // {
    //     auto hi = epsilon();
    //     auto lo = empty();
    //     return branch(x, std::move(hi), std::move(lo));
    // }

    // ZDD reduction rule:
    // If the high child is the zero node, then no subset
    // contains variable x. Therefore, the node labeled x
    // is redundant and can be eliminated
    auto branch(var_index x, edge_ptr&& hi, edge_ptr&& lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        // if hi == 0 then we eliminate the node
        if (hi == constant(0))
        {
            // we remove x from the zdd
            return lo;
        }

        // otherwise we create a zdd node
        return uedge(regw(), unode(x, std::move(hi), std::move(lo)));
    }


    auto union_set(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        assert(P && Q);

        // const-const: only {} (0) or {{}} (1)
        if (P->is_const() && Q->is_const())
        {
            return (P == constant(1) || Q == constant(1)) ? constant(1) : constant(0);
        }

        // trivial cases
        if (P == constant(0)) return Q;
        if (Q == constant(0)) return P;
        if (P == Q) return P;

        auto const x = top_var(P, Q);

        auto R0 = union_set(subset0(P, x), subset0(Q, x));
        auto R1 = union_set(subset1(P, x), subset1(Q, x));

        return branch(x, std::move(R1), std::move(R0));
    }

    auto inter_set(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        assert(P && Q);

        // const-const
        if (P->is_const() && Q->is_const())
        {
            return (P == constant(1) && Q == constant(1)) ? constant(1) : constant(0);
        }

        // trivial cases
        if (P == constant(0) || Q == constant(0)) return constant(0);
        if (P == Q) return P;

        auto const x = top_var(P, Q);

        auto R0 = inter_set(subset0(P, x), subset0(Q, x));
        auto R1 = inter_set(subset1(P, x), subset1(Q, x));

        return branch(x, std::move(R1), std::move(R0));
    }

    auto diff_set(edge_ptr const& P, edge_ptr const& Q) -> edge_ptr
    {
        assert(P && Q);

        // {} \ Q = {}
        if (P == constant(0)) return constant(0);

        // P \ {} = P
        if (Q == constant(0)) return P;

        // P \ P = {}
        if (P == Q) return constant(0);

        // const-const
        if (P->is_const() && Q->is_const())
        {
            // {{}} \ {} = {{}}, everything else => {}
            return (P == constant(1) && Q == constant(0)) ? constant(1) : constant(0);
        }

        auto const x = top_var(P, Q);

        auto R0 = diff_set(subset0(P, x), subset0(Q, x));
        auto R1 = diff_set(subset1(P, x), subset1(Q, x));

        return branch(x, std::move(R1), std::move(R0));
    }

};

//
// ====== zdd inline methods (copied skeleton from bdd.hpp) ======
//
inline auto zdd::operator&=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->apply_and(f, rhs.f);
    return *this;
}

inline auto zdd::operator|=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->apply_or(f, rhs.f);
    return *this;
}

inline auto zdd::operator-=(zdd const& rhs) -> zdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    f = mgr->apply_diff(f, rhs.f);
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

} //namespace freddy;