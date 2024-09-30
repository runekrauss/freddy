#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/conj.hpp"         // op::conj
#include "freddy/op/repl.hpp"         // op::repl

#include <algorithm>    // std::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cstddef>      // std::size_t
#include <functional>   // std::function
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
#include <optional>     // std::optional
#include <ostream>      // std::ostream
#include <sstream>      // std::ostringstream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::move
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Types
// =====================================================================================================================

class bhd_manager;

class bhd  // binary hybrid diagram
{
  public:
    bhd() = default;  // so that BHDs initially work with standard containers

    auto operator~() const;

    auto operator&=(bhd const&) -> bhd&;

    auto operator|=(bhd const&) -> bhd&;

    auto operator^=(bhd const&) -> bhd&;

    auto friend operator&(bhd lhs, bhd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(bhd lhs, bhd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(bhd lhs, bhd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(bhd const& lhs, bhd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BHD manager

        return lhs.f == rhs.f;
    }

    auto friend operator!=(bhd const& lhs, bhd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bhd const& g) -> std::ostream&
    {
        s << "Wrapper = " << g.f;
        s << "\nBHD manager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(bhd const& g) const noexcept
    {
        assert(f);

        return f->v == g.f->v;
    }

    [[nodiscard]] auto is_complemented() const noexcept
    {
        assert(f);

        return f->w;
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        assert(f);

        return f->v->is_const();
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto is_exp() const noexcept;  // Is there an expansion path (EXP) for SAT solving?

    [[nodiscard]] auto var() const
    {
        assert(!is_const());

        return f->v->br().x;
    }

    [[nodiscard]] auto high() const;

    [[nodiscard]] auto low() const;

    template <typename T, typename... Ts>
    auto fn(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto has_const(bool) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(bhd const&, bhd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bhd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    [[nodiscard]] auto sat() const;  // one existing solution per path

    auto gen_uclauses(std::ostream& = std::cout) const;  // unit clauses per EXP for DIMACS CNF separated by "c"

    auto print() const;

  private:
    friend bhd_manager;

    // wrapper is controlled by its BHD manager
    bhd(std::shared_ptr<detail::edge<bool, bool>> f, bhd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, bool>> f;

    bhd_manager* mgr{};
};

enum struct bhd_heuristic : std::uint8_t  // to determine when EXPs are made
{
    LVL,  // BDD level
    MEM   // peak BDD size in KB
};

class bhd_manager : public detail::manager<bool, bool>
{
  public:
    friend bhd;

    bhd_manager() :
            manager{tmls()}
    {
        consts.push_back(make_const(false, true));  // last node in EXP is treated as a constant
        consts.push_back(make_const(true, true));   // for reasons of consistency
    }

    bhd_manager(bhd_heuristic const heur, std::size_t const cost) :
            bhd_manager{}
    {
        if (heur == bhd_heuristic::LVL)
        {
            this->heur = [this](auto const& f, auto const& g, auto const x) { return lvl_heur(f, g, x); };
        }
        else
        {
            this->heur = [this](auto const& f, auto const& g, auto const x) { return mem_heur(f, g, x); };
        }
        this->cost = cost;
    }

    auto var(std::string_view l = {})
    {
        return bhd{make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bhd{vars[i], this};
    }

    auto zero() noexcept
    {
        return bhd{consts[0], this};
    }

    auto one() noexcept
    {
        return bhd{consts[1], this};
    }

    auto exp() noexcept
    {
        return bhd{consts[2], this};
    }

    [[nodiscard]] auto size(std::vector<bhd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bhd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto print(std::vector<bhd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        std::ostringstream buf;  // for highlighting EXP
        to_dot(transform(fs), outputs, buf);

        auto dot = buf.str();
        detail::replace_all(dot, "[shape=box,style=filled,color=brown,fontcolor=white,label=\"1\"]",
                            "[shape=triangle,style=filled,color=darkviolet,fontcolor=white,label=\"EXP\"]");
        s << dot;
    }

  private:
    using bool_edge = detail::edge<bool, bool>;

    using bool_node = detail::node<bool, bool>;

    [[nodiscard]] auto is_exp(edge_ptr const& f) const noexcept
    {
        return f == consts[2] || f == consts[3];
    }

    auto sat(edge_ptr const& f, std::vector<bool>& path, bool const m, std::vector<std::vector<bool>>& sols) const
    {
        assert(f);
        assert(!path.empty());

        if (is_exp(f))
        {
            return;
        }

        if (f->v->is_const())
        {
            if (m)
            {  // complement bit is odd => satisfying solution
                sols.push_back(path);
            }
            return;
        }

        path[f->v->br().x] = false;  // truth value is independent of complemented edges
        sat(f->v->br().lo, path, comb(m, f->v->br().lo->w), sols);

        path[f->v->br().x] = true;
        sat(f->v->br().hi, path, comb(m, f->v->br().hi->w), sols);
    }

    [[nodiscard]] auto sat(edge_ptr const& f) const
    {
        assert(f);

        std::vector<std::vector<bool>> sols;  // initial variable ordering applies

        if (f == consts[1])
        {
            sols.emplace_back(var_count());  // assignment consisting of only "false" is one solution
        }
        else if (f != consts[0] && !is_exp(f))
        {                                         // collect solutions
            std::vector<bool> path(var_count());  // maximum depth
            sat(f, path, f->w, sols);
        }

        return sols;
    }

    auto gen_uclauses(edge_ptr const& f, std::vector<std::optional<bool>>& path, std::ostream& s)
    {
        assert(f);
        assert(!path.empty());

        if (is_exp(f))
        {
            for (auto i = 0; i < static_cast<std::int32_t>(path.size()); ++i)
            {
                if (path[i].has_value())
                {                                                          // output literal
                    s << (*path[i] ? i + 1 : -i - 1) << ' ' << 0 << '\n';  // end of clause
                }
            }

            s << "c\n";  // empty comment means the clauses relating to the next EXP if one still exists
            return;
        }

        if (f->v->is_const())
        {
            return;
        }

        path[f->v->br().x] = false;
        gen_uclauses(f->v->br().lo, path, s);

        path[f->v->br().x] = true;
        gen_uclauses(f->v->br().hi, path, s);

        path[f->v->br().x].reset();
    }

    auto gen_uclauses(edge_ptr const& f, std::ostream& s)
    {
        assert(f);

        if (!has_const(f, true))
        {  // there are no EXPs
            return;
        }

        std::vector<std::optional<bool>> path(var_count());  // not every variable must be on the path
        gen_uclauses(f, path, s);
    }

    auto repl(edge_ptr const& f, edge_ptr const& exp, bool const m = false)  // works with AND
    {  // redirect 1-paths in f to exp for compacting reasons
        assert(f);
        assert(is_exp(exp));

        if (is_exp(f))
        {
            return f == exp ? f : consts[0];
        }

        if (f->v->is_const())
        {
            if (m)
            {  // negation => decision is up to EXP
                return f == consts[0] ? (exp == consts[2] ? consts[3] : consts[2]) : f;
            }

            return f == consts[0] ? f : exp;  // overwriting => complement bit disappears automatically
        }

        op::repl op{f, m};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto hi = f->w ? repl(f->v->br().hi, exp, !m) : repl(f->v->br().hi, exp, m);
        auto lo = f->w ? repl(f->v->br().lo, exp, !m) : repl(f->v->br().lo, exp, m);

        // normalize if needed
        auto w = lo->w;
        if (w)
        {
            hi = complement(hi);
            lo = complement(lo);
        }
        w = f->w ? !w : w;  // due to bit flipping

        op.r = foa(std::make_shared<bool_edge>(
            w, foa(std::make_shared<bool_node>(f->v->br().x, std::move(hi), std::move(lo)))));
        return cache(std::move(op))->r;
    }

    auto compr(edge_ptr const& f, edge_ptr const& g, std::int32_t const x, bool const a)
    {  // EXPs remain at the same level for validation reasons
        assert(f);
        assert(g);
        assert(x == top_var(f, g));

        auto gx = cof(g, x, a);
        if (is_exp(gx) && !f->v->is_const() && f->v->br().x != x)
        {  // f is below g => hide in EXP
            return gx;
        }

        auto fx = cof(f, x, a);
        if (is_exp(fx) && !g->v->is_const() && g->v->br().x != x)
        {
            return fx;
        }

        return conj(fx, gx);
    }

    auto no_heur(edge_ptr const& f, edge_ptr const& g, std::int32_t const x) -> edge_ptr
    {  // conjunction without making EXPs
        assert(f);
        assert(g);
        assert(x == top_var(f, g));

        return make_branch(x, compr(f, g, x, true), compr(f, g, x, false));
    }

    auto lvl_heur(edge_ptr const& f, edge_ptr const& g, std::int32_t const x) -> edge_ptr
    {  // heuristic that makes EXPs from a predetermined BDD level
        assert(f);
        assert(g);
        assert(x == top_var(f, g));

        if (f->v->is_const() || f->v->br().x < static_cast<std::int32_t>(cost))
        {
            return g->v->is_const() || g->v->br().x < static_cast<std::int32_t>(cost)
                       ? make_branch(x, compr(f, g, x, true), compr(f, g, x, false))
                       : repl(f, consts[2]);
        }
        return g->v->is_const() || g->v->br().x < static_cast<std::int32_t>(cost) ? repl(g, consts[2]) : consts[2];
    }

    auto mem_heur(edge_ptr const& f, edge_ptr const& g, std::int32_t const x) -> edge_ptr
    {  // heuristic that makes EXPs when a peak BDD size (nodes and edges) in KB is reached
        assert(f);
        assert(g);
        assert(x == top_var(f, g));

        if (((static_cast<float>(node_count()) * sizeof(bool_node) +
              static_cast<float>(edge_count()) * sizeof(bool_edge)) /
             1e3f) >= static_cast<float>(cost))
        {
            return repl(f, consts[2]);  // to ensure canonicity and because f is usually larger than g
        }
        return make_branch(x, compr(f, g, x, true), compr(f, g, x, false));
    }

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        auto const fg = complement(conj(f, g));
        return complement(conj(complement(conj(f, fg)), complement(conj(g, fg))));  // stands for XOR
    }

    [[nodiscard]] auto agg(bool const& w, bool const& val) const noexcept -> bool override
    {
        return !(w == val);
    }

    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const noexcept -> bool override
    {
        return !(w1 == w2);
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return !f->w ? foa(std::make_shared<bool_edge>(true, f->v)) : foa(std::make_shared<bool_edge>(false, f->v));
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        // constant terminal cases
        if (f == consts[0] || g == consts[0])
        {
            return consts[0];
        }
        if (f == consts[1])
        {
            return g;
        }
        if (g == consts[1])
        {
            return f;
        }
        if (f->v == g->v)
        {
            if (f->w == g->w)
            {
                return f;
            }
            if (has_const(f, true))
            {
                return consts[2];
            }
            return consts[0];
        }

        // EXP terminal cases
        if (is_exp(f) && is_exp(g))
        {
            return f == g ? f : consts[0];
        }
        if (is_exp(f))
        {
            return repl(g, f);
        }
        if (is_exp(g))
        {
            return repl(f, g);
        }

        op::conj op{f, g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        op.r = heur(f, g, top_var(f, g));
        return cache(std::move(op))->r;
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return complement(conj(complement(f), complement(g)));
    }

    auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == lo)
        {
            return hi;
        }

        auto const w = lo->w;
        return foa(
            std::make_shared<bool_edge>(w, foa(std::make_shared<bool_node>(x, !w ? std::move(hi) : complement(hi),
                                                                           !w ? std::move(lo) : complement(lo)))));
    }

    [[nodiscard]] auto merge(bool const& val1, bool const& val2) const noexcept -> bool override
    {
        return !(val1 == val2);
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return conj(f, g);
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;
    }

    auto static transform(std::vector<bhd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        auto const leaf = std::make_shared<bool_node>(false);
        return std::array<edge_ptr, 2>{std::make_shared<bool_edge>(false, leaf),
                                       std::make_shared<bool_edge>(true, leaf)};
    }

    // heuristic conjunction used to decrease BDDs
    std::function<edge_ptr(edge_ptr const&, edge_ptr const&, std::int32_t)> heur{
        [this](auto const& f, auto const& g, auto const x) { return no_heur(f, g, x); }};

    std::size_t cost{};  // determine when EXPs are made depending on the heuristic
};

auto inline bhd::operator~() const
{
    assert(mgr);

    return bhd{mgr->complement(f), mgr};
}

auto inline bhd::operator&=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline bhd::operator|=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline bhd::operator^=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->add(f, rhs.f);
    return *this;
}

auto inline bhd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

auto inline bhd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

auto inline bhd::is_exp() const noexcept
{
    assert(mgr);

    return mgr->is_exp(f);
}

auto inline bhd::high() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return bhd{f->v->br().hi, mgr};
}

auto inline bhd::low() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return bhd{f->v->br().lo, mgr};
}

template <typename T, typename... Ts>
auto inline bhd::fn(T const a, Ts... args) const
{
    assert(mgr);

    return bhd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline bhd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bhd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bhd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bhd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    // since the last node in EXP is treated as a constant (1)
    auto exp_is_reached = [&as, this](auto const& f) {
        auto trv = [&as, this](auto const& self, auto const& f) {
            assert(f);

            if (mgr->is_exp(f))
            {
                return true;
            }
            if (f->v->is_const())
            {
                return false;
            }
            return as[f->v->br().x] ? self(self, f->v->br().hi) : self(self, f->v->br().lo);
        };

        return trv(trv, f);
    };

    return exp_is_reached(f) ? std::nullopt : std::make_optional(mgr->eval(f, as));
}

auto inline bhd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline bhd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline bhd::ite(bhd const& g, bhd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return bhd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline bhd::compose(std::int32_t const x, bhd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bhd{mgr->compose(f, x, g.f), mgr};
}

auto inline bhd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bhd{mgr->restr(f, x, a), mgr};
}

auto inline bhd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->exist(f, x), mgr};
}

auto inline bhd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->forall(f, x), mgr};
}

auto inline bhd::sat() const
{
    assert(mgr);

    return mgr->sat(f);
}

auto inline bhd::gen_uclauses(std::ostream& s) const
{
    assert(mgr);

    mgr->gen_uclauses(f, s);
}

auto inline bhd::print() const
{
    assert(mgr);

    mgr->print({*this});
}

}  // namespace freddy::dd
