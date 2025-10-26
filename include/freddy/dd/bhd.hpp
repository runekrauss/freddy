#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                    // config
#include "freddy/detail/manager.hpp"            // detail::manager
#include "freddy/detail/node.hpp"               // detail::intrusive_edge_ptr
#include "freddy/detail/operation/conj.hpp"     // detail::conj
#include "freddy/detail/operation/replace.hpp"  // detail::replace
#include "freddy/expansion.hpp"                 // expansion::S

#include <boost/algorithm/string.hpp>  // boost::replace_all

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint8_t
#include <functional>   // std::function
#include <iostream>     // std::cout
#include <optional>     // std::optional
#include <ostream>      // std::ostream
#include <sstream>      // std::ostringstream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::pair
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

class bhd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class bhd final  // binary hybrid diagram between BDD and SAT
{
  public:
    bhd() noexcept = default;  // enable default BHD construction for compatibility with standard containers

    auto operator~() const;

    auto operator&=(bhd const&) -> bhd&;

    auto operator|=(bhd const&) -> bhd&;

    auto operator^=(bhd const&) -> bhd&;

    friend auto operator&(bhd lhs, bhd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    friend auto operator|(bhd lhs, bhd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend auto operator^(bhd lhs, bhd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    friend auto operator==(bhd const& lhs, bhd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BHD manager

        return lhs.f == rhs.f;
    }

    friend auto operator!=(bhd const& lhs, bhd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend auto operator<<(std::ostream& os, bhd const& g) -> std::ostream&
    {
        os << "BHD handle: " << g.f << '\n';
        os << "BHD manager: " << g.mgr;
        return os;
    }

    [[nodiscard]] auto same_node(bhd const& g) const noexcept
    {
        assert(f);
        assert(mgr == g.mgr);  // BHD g is valid in any case

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

        return bhd{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return bhd{f->ch()->br().lo, mgr};
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto is_exp() const noexcept;  // Is there an expansion for SAT solving?

    template <typename TruthValue, typename... TruthValues>
    auto fn(TruthValue, TruthValues...) const;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto ite(bhd const&, bhd const&) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto has_const(bool) const;

    [[nodiscard]] auto has_exp() const;

    [[nodiscard]] auto is_essential(var_index) const noexcept;

    [[nodiscard]] auto compose(var_index, bhd const&) const;

    [[nodiscard]] auto restr(var_index, bool) const;

    [[nodiscard]] auto exist(var_index) const;

    [[nodiscard]] auto forall(var_index) const;

    [[nodiscard]] auto sat_solutions() const;  // one existing solution per path

    [[nodiscard]] auto unit_clauses() const;  // for each expansion path to solve subfunctions via a SAT solver

    auto dump_dot(std::ostream& = std::cout) const;

  private:
    friend bhd_manager;

    // wrapper is controlled by its BHD manager
    bhd(detail::intrusive_edge_ptr<bool, bool> f, bhd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(this->mgr);
    }

    detail::intrusive_edge_ptr<bool, bool> f;  // BHD handle

    bhd_manager* mgr{};  // must be destroyed after this BHD wrapper
};

enum struct bhd_heuristic : std::uint8_t  // to determine when expansion paths are created
{
    LEVEL,  // BDD level
    MEMORY  // shared BDD size in bytes
};

class bhd_manager final : public detail::manager<bool, bool>
{
  public:
    // behavior similar to that of a BDD
    explicit bhd_manager(struct config const cfg = {}) :
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) because BHD terminals are intrusive
            manager{tmls(), cfg}
    {
        manager::constant(false, true, true);  // The last and only expansion node is treated as a constant.
        manager::constant(true, true, true);   // for reasons of consistency
    }

    bhd_manager(bhd_heuristic const heur, std::size_t const exp_thresh, struct config const cfg = {}) :
            bhd_manager{cfg}
    {
        if (heur == bhd_heuristic::LEVEL)
        {
            this->heur = [this](auto const& f, auto const& g, auto const x) { return level_heur(f, g, x); };
        }
        else  // memory heuristic
        {
            this->heur = [this](auto const& f, auto const& g, auto const x) { return memory_heur(f, g, x); };
        }
        this->exp_thresh = exp_thresh;
    }

    auto var(std::string_view lbl = {})
    {
        return bhd{manager::var(expansion::S, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        return bhd{manager::var(x), this};
    }

    auto zero() noexcept
    {
        return bhd{constant(0), this};
    }

    auto one() noexcept
    {
        return bhd{constant(1), this};
    }

    auto exp() noexcept
    {
        return bhd{constant(2), this};
    }

    [[nodiscard]] auto size(std::vector<bhd> const& fs) const
    {
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bhd> const& fs) const
    {
        assert(!fs.empty());

        return manager::depth(transform(fs));
    }

    auto dump_dot(std::vector<bhd> const& fs, std::vector<std::string> const& outputs = {},
                  std::ostream& os = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        // to highlight the expansion node labeled with "EXP" that marks the end of all expansion paths
        std::ostringstream oss;
        manager::dump_dot(transform(fs), outputs, oss);

        auto dot = oss.str();
        boost::replace_all(dot, "[shape=box,style=filled,color=chocolate,fontcolor=white,label=\"1\"]",
                           "[shape=triangle,style=filled,color=darkviolet,fontcolor=white,label=\"EXP\"]");
        os << dot;
    }

  private:
    friend bhd;

    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        node_ptr const leaf{new node{false}};
        return {edge_ptr{new edge{false, leaf}}, edge_ptr{new edge{true, leaf}}};
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    static auto transform(std::vector<bhd> const& gs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }

    [[nodiscard]] auto is_exp(edge_ptr const& f) const noexcept
    {
        return f == constant(2) || f == constant(3);
    }

    // since the expansion node is treated as the 1-leaf
    [[nodiscard]] auto eval(edge_ptr const& f, std::vector<bool> const& as) const noexcept
    {
        assert(as.size() == var_count());

        auto exp_is_reached = [&as, this](edge_ptr const& e) {
            auto trav = [&as, this](auto const& self, edge_ptr const& e) {  // traversal
                assert(e);

                if (is_exp(e))
                {  // The function value cannot be determined at this point.
                    return true;
                }
                if (e->is_const())
                {
                    return false;
                }
                return as[e->ch()->br().x] ? self(self, e->ch()->br().hi) : self(self, e->ch()->br().lo);
            };

            return trav(trav, e);
        };

        return exp_is_reached(f) ? std::nullopt : std::optional{manager::eval(f, as)};
    }

    auto sat_solutions(edge_ptr const& f, bool const m, std::vector<bool>& path,
                       std::vector<std::vector<bool>>& sols) const
    {
        if (is_exp(f))
        {
            return;
        }

        if (f->is_const())
        {
            if (m)
            {  // mark (complement bit) is "odd" => satisfying solution
                sols.push_back(path);
            }
            return;
        }

        path[f->ch()->br().x] = false;
        sat_solutions(f->ch()->br().lo, comb(m, f->ch()->br().lo->weight()), path, sols);

        path[f->ch()->br().x] = true;
        sat_solutions(f->ch()->br().hi, comb(m, f->ch()->br().hi->weight()), path, sols);
    }

    [[nodiscard]] auto sat_solutions(edge_ptr const& f) const
    {
        assert(f);

        std::vector<std::vector<bool>> sols;  // where the initial variable order applies

        if (f == constant(1))
        {
            sols.emplace_back(var_count());  // assignment consisting only of "false" is one solution
        }
        else if (f != constant(0) && !is_exp(f))
        {
            sols.reserve(var_count());
            std::vector<bool> path(var_count());        // don't care terms at maximum depth
            sat_solutions(f, f->weight(), path, sols);  // collect simultaneously represented solutions
        }

        return sols;
    }

    auto unit_clauses(edge_ptr const& f, std::vector<std::optional<bool>>& exp_path,
                      std::vector<std::vector<std::pair<var_index, bool>>>& ucs) const
    {
        if (is_exp(f))
        {
            std::vector<std::pair<var_index, bool>> path;
            for (auto i = 0uz; i < exp_path.size(); ++i)
            {
                if (exp_path[i])
                {  // variable has been encountered
                    path.emplace_back(static_cast<var_index>(i), *exp_path[i]);
                }
            }
            ucs.push_back(std::move(path));

            return;
        }

        if (f->is_const())
        {
            return;
        }

        exp_path[f->ch()->br().x] = false;
        unit_clauses(f->ch()->br().lo, exp_path, ucs);

        exp_path[f->ch()->br().x] = true;  // truth value is independent of complemented edges
        unit_clauses(f->ch()->br().hi, exp_path, ucs);

        exp_path[f->ch()->br().x].reset();
    }

    auto unit_clauses(edge_ptr const& f)
    {
        assert(f);

        std::vector<std::vector<std::pair<var_index, bool>>> ucs;  // generated unit clauses

        if (!has_const(f, true))
        {  // there are no expansion paths
            return ucs;
        }

        ucs.reserve(var_count());
        std::vector<std::optional<bool>> exp_path(var_count());  // not every variable must be on a path
        unit_clauses(f, exp_path, ucs);

        return ucs;
    }

    auto replace(edge_ptr const& f, bool const m = false)
    {  // redirect 1-paths to the expansion node for compactness reasons
        if (is_exp(f))
        {
            return f;
        }

        if (f->is_const())
        {
            if (m)
            {
                return f == constant(0) ? constant(2) : f;
            }

            return f == constant(0) ? f : constant(2);  // expansion node
        }

        detail::replace op{f, m};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto hi = f->weight() ? replace(f->ch()->br().hi, !m) : replace(f->ch()->br().hi, m);
        auto lo = f->weight() ? replace(f->ch()->br().lo, !m) : replace(f->ch()->br().lo, m);
        if (hi == lo)
        {
            op.set_result(hi);
            return cache(std::move(op))->get_result();
        }

        // normalize if needed
        auto w = lo->weight();
        if (w)
        {
            hi = complement(hi);
            lo = complement(lo);
        }
        w = f->weight() ? !w : w;  // bit flipping

        op.set_result(uedge(w, unode(f->ch()->br().x, std::move(hi), std::move(lo))));
        return cache(std::move(op))->get_result();
    }

    auto compress(edge_ptr const& f, edge_ptr const& g, var_index const x, bool const a)
    {  // preserve expansion paths for validation purposes
        auto fx = cof(f, x, a);
        if (is_exp(fx) && !g->is_const() && g->ch()->br().x != x)
        {  // g is below f => "hide" in the expansion node
            return fx;
        }

        auto gx = cof(g, x, a);
        if (is_exp(gx) && !f->is_const() && f->ch()->br().x != x)
        {
            return gx;
        }

        return conj(fx, gx);
    }

    auto no_heur(edge_ptr const& f, edge_ptr const& g, var_index const x) -> edge_ptr
    {  // do not restrict the solution space
        return branch(x, compress(f, g, x, true), compress(f, g, x, false));
    }

    auto level_heur(edge_ptr const& f, edge_ptr const& g, var_index const x) -> edge_ptr
    {  // restrict the solution space based on a predetermined level threshold
        if (f->is_const() || f->ch()->br().x < exp_thresh)
        {
            return g->is_const() || g->ch()->br().x < exp_thresh
                       ? branch(x, compress(f, g, x, true), compress(f, g, x, false))
                       : replace(f);
        }
        return g->is_const() || g->ch()->br().x < exp_thresh ? replace(g) : constant(2);  // expansion node
    }

    auto memory_heur(edge_ptr const& f, edge_ptr const& g, var_index const x) -> edge_ptr
    {  // restrict the solution space based on a predetermined memory threshold (shared BDD size)
        // no overflow protection, as the theoretical worst-case scenario merely leads to conjunction
        if (edge_count() * sizeof(edge) + node_count() * sizeof(node) >= exp_thresh)
        {
            return replace(f);  // because f is usually larger than g
        }
        return branch(x, compress(f, g, x, true), compress(f, g, x, false));  // conjunction
    }

    [[nodiscard]] auto agg(bool const& w, bool const& val) const noexcept -> bool override
    {
        return w != val;
    }

    auto branch(var_index const x, edge_ptr&& hi, edge_ptr&& lo) -> edge_ptr override
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

    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const noexcept -> bool override
    {
        return w1 != w2;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return f->weight() ? uedge(false, f->ch()) : uedge(true, f->ch());
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        // terminal cases regarding standard constants
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
        if (f->ch() == g->ch())
        {
            if (f->weight() == g->weight())
            {
                return f;
            }
            if (!has_const(f, true))  // The expansion node is never removed.
            {
                return constant(0);  // f & !f = 0
            }
        }

        // terminal cases regarding the expansion node
        if (is_exp(f))
        {
            return is_exp(g) ? constant(2) : replace(g);
        }
        if (is_exp(g))
        {
            return replace(f);
        }

        detail::conj op{f, g};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        op.set_result(heur(f, g, top_var(f, g)));
        return cache(std::move(op))->get_result();
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return complement(conj(complement(f), complement(g)));
    }

    [[nodiscard]] auto merge(bool const& val1, bool const& val2) const noexcept -> bool override
    {
        return val1 != val2;
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return conj(f, g);
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return disj(conj(complement(f), g), conj(f, complement(g)));  // stands for XOR
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;
    }

    // heuristic technique that can be used to reduce BDD sizes during conjunction
    std::function<edge_ptr(edge_ptr const&, edge_ptr const&, var_index)> heur{
        [this](auto const& f, auto const& g, auto const x) { return no_heur(f, g, x); }};

    // threshold value from which expansion paths are automatically created depending on the heuristic
    std::size_t exp_thresh{};
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

template <typename TruthValue, typename... TruthValues>
inline auto bhd::fn(TruthValue const a, TruthValues... as) const
{
    assert(mgr);

    return bhd{mgr->fn(f, a, std::forward<TruthValues>(as)...), mgr};
}

inline auto bhd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);

    return mgr->eval(f, as);
}

inline auto bhd::ite(bhd const& g, bhd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return bhd{mgr->ite(f, g.f, h.f), mgr};
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

inline auto bhd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

inline auto bhd::has_exp() const
{
    return has_const(true);
}

inline auto bhd::is_essential(var_index const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

inline auto bhd::compose(var_index const x, bhd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bhd{mgr->compose(f, x, g.f), mgr};
}

inline auto bhd::restr(var_index const x, bool const a) const
{
    assert(mgr);

    return bhd{mgr->restr(f, x, a), mgr};
}

inline auto bhd::exist(var_index const x) const
{
    assert(mgr);

    return bhd{mgr->exist(f, x), mgr};
}

inline auto bhd::forall(var_index const x) const
{
    assert(mgr);

    return bhd{mgr->forall(f, x), mgr};
}

inline auto bhd::sat_solutions() const
{
    assert(mgr);

    return mgr->sat_solutions(f);
}

inline auto bhd::unit_clauses() const
{
    assert(mgr);

    return mgr->unit_clauses(f);
}

inline auto bhd::dump_dot(std::ostream& os) const
{
    assert(mgr);

    mgr->dump_dot({*this}, {}, os);
}

}  // namespace freddy
