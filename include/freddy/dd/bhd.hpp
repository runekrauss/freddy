#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/conj.hpp"         // op::conj
#include "freddy/op/repl.hpp"         // op::repl

#include <boost/algorithm/string.hpp>  // boost::replace_all

#include <algorithm>    // std::ranges::transform
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
#include <utility>      // std::cmp_less
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
        s << "Wrapper: " << g.f;
        s << "\nBHD manager: " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(bhd const& g) const noexcept
    {
        assert(f);

        return f->ch() == g.f->ch();
    }

    [[nodiscard]] auto is_complemented() const noexcept
    {
        assert(f);

        return f->weight();
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        assert(f);

        return f->ch()->is_const();
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto is_exp() const noexcept;  // Is there an expansion path (EXP) for SAT solving?

    [[nodiscard]] auto var() const
    {
        assert(!is_const());

        return f->ch()->br().x;
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

    [[nodiscard]] auto uc() const;  // unit clauses per EXP for solving subfunctions by SAT

    auto print(std::ostream& = std::cout) const;

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
        make_const(false, true, true);  // last node in EXP is treated as a constant
        make_const(true, true, true);   // for reasons of consistency
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
        // assert(i < var_count());

        return bhd{get_var(i), this};
    }

    auto zero() noexcept
    {
        return bhd{get_const(0), this};
    }

    auto one() noexcept
    {
        return bhd{get_const(1), this};
    }

    auto exp() noexcept
    {
        return bhd{get_const(2), this};
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
        boost::replace_all(dot, "[shape=box,style=filled,color=chocolate,fontcolor=white,label=\"1\"]",
                           "[shape=triangle,style=filled,color=darkviolet,fontcolor=white,label=\"EXP\"]");
        s << dot;
    }

  private:
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        auto const leaf = std::make_shared<detail::node<bool, bool>>(false);
        return std::array<edge_ptr, 2>{std::make_shared<detail::edge<bool, bool>>(false, leaf),
                                       std::make_shared<detail::edge<bool, bool>>(true, leaf)};
    }

    static auto transform(std::vector<bhd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    [[nodiscard]] auto is_exp(edge_ptr const& f) const noexcept
    {
        return f == get_const(2) || f == get_const(3);
    }

    auto sat(edge_ptr const& f, std::vector<bool>& path, bool const m, std::vector<std::vector<bool>>& sols) const
    {
        assert(f);
        assert(!path.empty());

        if (is_exp(f))
        {
            return;
        }

        if (f->ch()->is_const())
        {
            if (m)
            {  // complement bit is odd => satisfying solution
                sols.push_back(path);
            }
            return;
        }

        path[f->ch()->br().x] = false;  // truth value is independent of complemented edges
        sat(f->ch()->br().lo, path, comb(m, f->ch()->br().lo->weight()), sols);

        path[f->ch()->br().x] = true;
        sat(f->ch()->br().hi, path, comb(m, f->ch()->br().hi->weight()), sols);
    }

    [[nodiscard]] auto sat(edge_ptr const& f) const
    {
        assert(f);

        std::vector<std::vector<bool>> sols;  // initial variable ordering applies

        if (f == get_const(1))
        {
            sols.emplace_back(var_count());  // assignment consisting of only "false" is one solution
        }
        else if (f != get_const(0) && !is_exp(f))
        {                                         // collect solutions
            std::vector<bool> path(var_count());  // maximum depth
            sat(f, path, f->weight(), sols);
        }

        return sols;
    }

    auto uc(edge_ptr const& f, std::vector<std::optional<bool>>& path,
            std::vector<std::vector<std::pair<std::int32_t, bool>>>& uclauses) const
    {
        assert(f);

        if (is_exp(f))
        {
            std::vector<std::pair<std::int32_t, bool>> tmp;
            for (decltype(path.size()) i = 0; i < path.size(); ++i)
            {
                if (path[i].has_value())
                {
                    tmp.emplace_back(static_cast<std::int32_t>(i), *path[i]);
                }
            }
            uclauses.push_back(std::move(tmp));

            return;
        }

        if (f->ch()->is_const())
        {
            return;
        }

        path[f->ch()->br().x] = false;
        uc(f->ch()->br().lo, path, uclauses);

        path[f->ch()->br().x] = true;
        uc(f->ch()->br().hi, path, uclauses);

        path[f->ch()->br().x].reset();
    }

    auto uc(edge_ptr const& f)
    {
        assert(f);

        std::vector<std::vector<std::pair<std::int32_t, bool>>> uclauses;

        if (!has_const(f, true))
        {  // there are no EXPs
            return uclauses;
        }

        std::vector<std::optional<bool>> path(var_count());  // not every variable must be on the path
        uc(f, path, uclauses);

        return uclauses;
    }

    auto repl(edge_ptr const& f, bool const m = false)  // works with AND
    {                                                   // redirect 1-paths in f to exp for compacting reasons
        assert(f);

        if (is_exp(f))
        {
            return f;
        }

        if (f->ch()->is_const())
        {
            if (m)
            {
                return f == get_const(0) ? get_const(2) : f;
            }

            return f == get_const(0) ? f : get_const(2);
        }

        op::repl op{f, m};
        if (auto const* const ent = cached(op))
        {
            return ent->result();
        }

        auto hi = f->weight() ? repl(f->ch()->br().hi, !m) : repl(f->ch()->br().hi, m);
        auto lo = f->weight() ? repl(f->ch()->br().lo, !m) : repl(f->ch()->br().lo, m);
        if (hi == lo)
        {
            op.result() = hi;
            return cache(std::move(op))->result();
        }

        // normalize if needed
        auto w = lo->weight();
        if (w)
        {
            hi = complement(hi);
            lo = complement(lo);
        }
        w = f->weight() ? !w : w;  // due to bit flipping

        op.result() = uedge(w, unode(f->ch()->br().x, std::move(hi), std::move(lo)));
        return cache(std::move(op))->result();
    }

    auto compr(edge_ptr const& f, edge_ptr const& g, detail::var_index const x, bool const a)
    {  // EXPs remain at the same level for validation reasons
        assert(f);
        assert(g);
        // assert(x == top_var(f, g));

        auto gx = cof(g, x, a);
        if (is_exp(gx) && !f->ch()->is_const() && f->ch()->br().x != x)
        {  // f is below g => hide in EXP
            return gx;
        }

        auto fx = cof(f, x, a);
        if (is_exp(fx) && !g->ch()->is_const() && g->ch()->br().x != x)
        {
            return fx;
        }

        return conj(fx, gx);
    }

    auto no_heur(edge_ptr const& f, edge_ptr const& g, std::int32_t const x) -> edge_ptr
    {  // conjunction without making EXPs
        assert(f);
        assert(g);
        // assert(x == top_var(f, g));

        return make_branch(x, compr(f, g, x, true), compr(f, g, x, false));
    }

    auto lvl_heur(edge_ptr const& f, edge_ptr const& g, std::int32_t const x) -> edge_ptr
    {  // heuristic that makes EXPs from a predetermined BDD level
        assert(f);
        assert(g);
        // assert(x == top_var(f, g));

        if (f->ch()->is_const() || std::cmp_less(f->ch()->br().x, cost))
        {
            return g->ch()->is_const() || std::cmp_less(g->ch()->br().x, cost)
                       ? make_branch(x, compr(f, g, x, true), compr(f, g, x, false))
                       : repl(f);
        }
        return g->ch()->is_const() || std::cmp_less(g->ch()->br().x, cost) ? repl(g) : get_const(2);
    }

    auto mem_heur(edge_ptr const& f, edge_ptr const& g, std::int32_t const x) -> edge_ptr
    {  // heuristic that makes EXPs when a peak BDD size (nodes and edges) in KB is reached
        assert(f);
        assert(g);
        // assert(x == top_var(f, g));

        if (((static_cast<float>(node_count()) * sizeof(detail::node<bool, bool>) +
              static_cast<float>(edge_count()) * sizeof(detail::edge<bool, bool>)) /
             1e3f) >= static_cast<float>(cost))
        {
            return repl(f);  // to ensure canonicity and because f is usually larger than g
        }
        return make_branch(x, compr(f, g, x, true), compr(f, g, x, false));
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return disj(conj(complement(f), g), conj(f, complement(g)));  // stands for XOR
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

        return !f->weight() ? uedge(true, f->ch()) : uedge(false, f->ch());
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        // constant terminal cases
        if (f == get_const(0) || g == get_const(0))
        {
            return get_const(0);
        }
        if (f == get_const(1))
        {
            return g;
        }
        if (g == get_const(1))
        {
            return f;
        }
        if (f->ch() == g->ch())
        {
            if (f->weight() == g->weight())
            {
                return f;
            }
            if (!has_const(f, true))
            {  // f & !f = 0
                return get_const(0);
            }  // EXP is never removed
        }

        // EXP terminal cases
        if (is_exp(f))
        {
            return is_exp(g) ? get_const(2) : repl(g);
        }
        if (is_exp(g))
        {
            return repl(f);
        }

        op::conj op{f, g};
        if (auto const* const ent = cached(op))
        {
            return ent->result();
        }

        op.result() = heur(f, g, top_var(f, g));
        return cache(std::move(op))->result();
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return complement(conj(complement(f), complement(g)));
    }

    auto make_branch(detail::var_index const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == lo)
        {
            return hi;
        }

        auto const w = lo->weight();
        return uedge(w, unode(x, !w ? std::move(hi) : complement(hi), !w ? std::move(lo) : complement(lo)));
    }

    auto merge(bool const& val1, bool const& val2) const noexcept -> bool override
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

    // heuristic conjunction used to decrease BDDs
    std::function<edge_ptr(edge_ptr const&, edge_ptr const&, detail::var_index)> heur{
        [this](auto const& f, auto const& g, auto const x) { return no_heur(f, g, x); }};

    std::size_t cost{};  // determine when EXPs are made depending on the heuristic
};

inline auto bhd::operator~() const
{
    assert(mgr);

    return bhd{mgr->complement(f), mgr};
}

inline auto bhd::operator&=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

inline auto bhd::operator|=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

inline auto bhd::operator^=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->plus(f, rhs.f);
    return *this;
}

inline auto bhd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

inline auto bhd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

inline auto bhd::is_exp() const noexcept
{
    assert(mgr);

    return mgr->is_exp(f);
}

inline auto bhd::high() const
{
    assert(mgr);
    assert(!f->ch()->is_const());

    return bhd{f->ch()->br().hi, mgr};
}

inline auto bhd::low() const
{
    assert(mgr);
    assert(!f->ch()->is_const());

    return bhd{f->ch()->br().lo, mgr};
}

template <typename T, typename... Ts>
inline auto bhd::fn(T const a, Ts... args) const
{
    assert(mgr);

    return bhd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

inline auto bhd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

inline auto bhd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

inline auto bhd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

inline auto bhd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(std::cmp_equal(as.size(), mgr->var_count()));

    // since the last node in EXP is treated as a constant (1)
    auto exp_is_reached = [&as, this](auto const& f) {
        auto trv = [&as, this](auto const& self, auto const& f) {
            assert(f);

            if (mgr->is_exp(f))
            {
                return true;
            }
            if (f->ch()->is_const())
            {
                return false;
            }
            return as[f->ch()->br().x] ? self(self, f->ch()->br().hi) : self(self, f->ch()->br().lo);
        };

        return trv(trv, f);
    };

    return exp_is_reached(f) ? std::nullopt : std::make_optional(mgr->eval(f, as));
}

inline auto bhd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

inline auto bhd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

inline auto bhd::ite(bhd const& g, bhd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return bhd{mgr->ite(f, g.f, h.f), mgr};
}

inline auto bhd::compose(std::int32_t const x, bhd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bhd{mgr->compose(f, x, g.f), mgr};
}

inline auto bhd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bhd{mgr->restr(f, x, a), mgr};
}

inline auto bhd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->exist(f, x), mgr};
}

inline auto bhd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->forall(f, x), mgr};
}

inline auto bhd::sat() const
{
    assert(mgr);

    return mgr->sat(f);
}

inline auto bhd::uc() const
{
    assert(mgr);

    return mgr->uc(f);
}

inline auto bhd::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
