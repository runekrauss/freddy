#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                  // config
#include "freddy/detail/manager.hpp"          // detail::manager
#include "freddy/detail/operation/conj.hpp"   // detail::conj
#include "freddy/expansion.hpp"               // expansion::S

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <iostream>     // std::cout
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::forward
#include <vector>       // std::vector

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

    // negation (Complement)
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

    friend auto operator==(zdd const& lhs, zdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);
        return lhs.f == rhs.f;
    }

    friend auto operator!=(zdd const& lhs, zdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    [[nodiscard]] auto same_node(zdd const& g) const noexcept
    {
        assert(f);
        assert(mgr == g.mgr);

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

    [[nodiscard]] auto eval(std::vector<bool> const& as) const noexcept -> bool;

    [[nodiscard]] auto size() const;
    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto is_essential(var_index) const noexcept;

    [[nodiscard]] auto compose(var_index, zdd const&) const;

    [[nodiscard]] auto restr(var_index, bool) const;

    [[nodiscard]] auto exist(var_index) const;

    [[nodiscard]] auto forall(var_index) const;

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

        op.set_result(uedge(regw(), unode(x, conj(cof(f, x, true), cof(g, x, true)), conj(cof(f, x, false), cof(g, x, false)))));
        return cache(std::move(op))->get_result();
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        auto const rec = [this](auto&& self, edge_ptr const& ff, edge_ptr const& gg,
                                var_index const lvl) -> edge_ptr
        {
            if (lvl >= static_cast<var_index>(var_count()))
            {
                return (ff == constant(0) && gg == constant(0)) ? constant(0) : constant(1);
            }

            auto const f_hi = (!ff->is_const() && ff->ch()->br().x == lvl) ? denorm_high(ff) : ff;
            auto const f_lo = (!ff->is_const() && ff->ch()->br().x == lvl) ? denorm_low(ff) : ff;
            auto const g_hi = (!gg->is_const() && gg->ch()->br().x == lvl) ? denorm_high(gg) : gg;
            auto const g_lo = (!gg->is_const() && gg->ch()->br().x == lvl) ? denorm_low(gg) : gg;

            auto hi = self(self, f_hi, g_hi, static_cast<var_index>(lvl + 1));
            auto lo = self(self, f_lo, g_lo, static_cast<var_index>(lvl + 1));

            return branch(lvl, std::move(hi), std::move(lo));
        };

        return rec(rec, f, g, 0);
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        if (f == constant(0))
        {
            return constant(1);
        }
        if (f == constant(1))
        {
            return constant(0);
        }

        auto const x = f->ch()->br().x;
        return uedge(regw(), unode(x, complement(denorm_high(f)), complement(denorm_low(f))));
    }

    auto path_count(edge_ptr const& f) -> double
    {
        if (f == constant(0))
        {
            return 0.0;
        }
        if (f == constant(1))
        {
            return 1.0;
        }

        return path_count(f->ch()->br().hi) + path_count(f->ch()->br().lo);
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return conj(f, g);
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return disj(f, g);
    }

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

    [[nodiscard]] auto reducible(edge_ptr const& hi, edge_ptr const&, expansion) const -> bool override
    {
        return hi == constant(0);
    }
    [[nodiscard]] auto reduced(edge_ptr const&, edge_ptr const& lo, expansion) -> edge_ptr override
    {
        return lo;
    }
    [[nodiscard]] auto expanded(edge_ptr const& f, bool const, expansion) const -> edge_ptr override
    {
        return f;
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

inline auto zdd::eval(std::vector<bool> const& as) const noexcept -> bool
{
    assert(mgr);
    assert(as.size() == mgr->var_count());

    auto cur = *this;

    while (!cur.is_const())
    {
        auto const x = static_cast<std::size_t>(cur.var());
        cur = as[x] ? cur.high() : cur.low();
    }

    return cur.is_one();
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

inline auto zdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

inline auto zdd::is_essential(var_index const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

inline auto zdd::compose(var_index const x, zdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return zdd{mgr->compose(f, x, g.f), mgr};
}

inline auto zdd::restr(var_index const x, bool const a) const
{
    assert(mgr);

    return zdd{mgr->restr(f, x, a), mgr};
}

inline auto zdd::exist(var_index const x) const
{
    assert(mgr);

    return zdd{mgr->exist(f, x), mgr};
}

inline auto zdd::forall(var_index const x) const
{
    assert(mgr);

    return zdd{mgr->forall(f, x), mgr};
}

inline auto zdd::dump_dot(std::ostream& os) const
{
    assert(mgr);
    mgr->dump_dot({*this}, {}, os);
}


}  // namespace freddy