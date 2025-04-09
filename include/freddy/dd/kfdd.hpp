#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/antiv.hpp"        // op::antiv
#include "freddy/op/conj.hpp"         // op::conj
#include "freddy/op/ite.hpp"          // op::ite
#include "freddy/op/sharpsat.hpp"     // op::sharpsat

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::pow
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
#include <ostream>      // std::ostream
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

class kfdd_manager;

class kfdd  // binary decision diagram
{
  public:
    kfdd() = default;  // so that kfdds initially work with standard containers

    auto operator~() const;

    auto operator&=(kfdd const&) -> kfdd&;

    auto operator|=(kfdd const&) -> kfdd&;

    auto operator^=(kfdd const&) -> kfdd&;

    auto friend operator&(kfdd lhs, kfdd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(kfdd lhs, kfdd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(kfdd lhs, kfdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(kfdd const& lhs, kfdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same kfdd manager

        return lhs.f == rhs.f;
    }

    auto friend operator!=(kfdd const& lhs, kfdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, kfdd const& g) -> std::ostream&
    {
        s << "Wrapper: " << g.f;
        s << "\nkfdd manager: " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(kfdd const& g) const noexcept
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

    [[nodiscard]] auto ite(kfdd const&, kfdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, kfdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    [[nodiscard]] auto sharpsat() const;

    auto manager() const noexcept -> kfdd_manager const&
    {
        return *mgr;
    }

    auto print(std::ostream& = std::cout) const;

  private:
    friend kfdd_manager;

    // wrapper is controlled by its kfdd manager
    kfdd(std::shared_ptr<detail::edge<bool, bool>> f, kfdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, bool>> f;  // DD handle

    kfdd_manager* mgr{};  // must be destroyed after this wrapper
};

class kfdd_manager : public detail::manager<bool, bool>
{
  public:
    friend kfdd;

    kfdd_manager() :
            manager{tmls()}
    {}

    auto var(freddy::expansion t, std::string_view l = {})
    {
        return kfdd{make_var(t, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return kfdd{vars[i], this};
    }

    auto zero() noexcept
    {
        return kfdd{consts[0], this};
    }

    auto one() noexcept
    {
        return kfdd{consts[1], this};
    }

    [[nodiscard]] auto size(std::vector<kfdd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<kfdd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto print(std::vector<kfdd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        to_dot(transform(fs), outputs, s);
    }

    auto dtl_sift()
    {
        gc();

        auto comp_largest_layer = [&](const int x, const int y) { return vl[x].nt.size() > vl[y].nt.size(); };

        gc();
#ifndef NDEBUG
        std::cout << "Before sifting:" << '\n';
        for (auto x = 0; x < var_count(); x++)
        {
            std::cout << "var: " << std::format("{:03}", x) << ", t: " << e_to_s(vl[x].t)
                      << ", pos: " << std::format("{:03}", var2lvl[x]) << "\n";
        }
#endif

        auto tmp_vars = std::vector<int>(var_count());
        for (auto x = 0; x < var_count(); x++)
        {
            tmp_vars[x] = x;
        }
        std::ranges::sort(tmp_vars, comp_largest_layer);
        for (auto i = 0; i < var_count(); ++i)
        {
            auto x = tmp_vars[i];
            sift_single_var(x);
        }

#ifndef NDEBUG
        std::cout << "After sifting:" << '\n';
        for (auto x = 0; x < var_count(); x++)
        {
            std::cout << "var: " << std::format("{:03}", x) << ", t: " << e_to_s(vl[x].t)
                      << ", pos: " << std::format("{:03}", var2lvl[x]) << "\n";
        }
#endif
    }

    void change_expansion_type(int x, expansion t)
    {
        //remember original expansion type
        auto org_t = vl[x].t;
#ifndef NDEBUG
        //get level
        const int level = var2lvl[x];
        //ensure level is last layer
#endif
        assert(level == var_count() - 1);
        //ensure there is only one node on last layer
        assert(vl[x].nt.size() == 1);

        //same expansion type => skip
        if (org_t == t)
        {
            return;
        }

        //set new expansion type
        vl[x].t = t;

        //get only node
        auto const& node = vl[x].nt.begin()->get();
        //ensure not a constant
        assert(node->is_const() == false);
        //get branch
        auto& br = node->br();

        //ensure both children are consts
        assert(br.hi == consts[1]);
        assert(br.lo == consts[0]);

        //only on change from or to negative davio adjustments are needed
        if (t == expansion::ND || org_t == expansion::ND)
        {
            //invert all edges pointing to node
            for (auto const& edge : vl[br.x].et)
            {
                edge->w = agg(edge->w, true);
            }
            ensure_canonicity();
        }

        gc();
    }

  private:
    struct smallest_level_r
    {
        int x;
        int pos;
        size_t size;
        expansion exp;
    };

    auto move_to_bottom(const int x) -> void
    {
        sift(var2lvl[x], var_count() - 1);
    }

    auto find_smallest_level(const int x) -> smallest_level_r
    {
        move_to_bottom(x);
        auto size = get_size();
        auto pos = var2lvl[x];
        auto exceeding_size = static_cast<float>(get_size()) * config::growth_factor;
        for (auto i = pos - 1; i >= 0; --i)
        {
            exchange(i);
            auto current_size = get_size();
            auto new_size_f = static_cast<float>(current_size);
            if (new_size_f > exceeding_size)
            {
                exchange(i);
#ifndef NDEBUG
                std::cout << "aborting sifting of variable " << x << ", exceeding growth factor." << '\n';
#endif
                break;
            }
            if (current_size < size)
            {
                size = current_size;
                pos = i;
            }
        }
        return {.x = x, .pos = pos, .size = size, .exp = vl[x].t};
    }
    auto get_size() -> size_t
    {
        return static_cast<size_t>(node_count());
    }

    auto sift_single_var(const int x) -> smallest_level_r
    {
        // std::vector<expansion> allowed_expansions = {expansion::S, expansion::PD};
        // std::vector<expansion> exps = allowed_expansions;
        // if (exps.size() == 1)
        // {
        //     //reorder
        // }
        // else
        // {
        //     assert(std::find(exps.begin(), exps.end(), vl[x].t) != exps.end());
        //     //sifte variable mit aktuellem zerlegungstypen, erst nach oben dann nach unten
        //     smallest_level_r tmp_res{};
        //     smallest_level_r res{.x = x, .pos = var2lvl[x], .size = get_size(), .expansion = vl[x].t};
        //     //dtl sifting
        //     move_to_top(x);
        //     tmp_res = find_smallest_level_downwards(x);
        //     if (tmp_res.size < res.size)
        //     {
        //         res = tmp_res;
        //     }
        //     exps.erase(vl[x].t);
        //
        //     while (!exps.empty())
        //     {
        //         move_to_bottom(x);
        //         change_expansion_type(x, exps.pop_back());
        //         tmp_res = find_smallest_level(x);
        //         if (tmp_res.size < res.size)
        //         {
        //             res = tmp_res;
        //         }
        //     }
        // }

        smallest_level_r tmp_res{};
        smallest_level_r res{.x = x, .pos = var2lvl[x], .size = get_size(), .exp = vl[x].t};

        //find smallest level for Shannon
        move_to_bottom(x);
        change_expansion_type(x, expansion::S);
        tmp_res = find_smallest_level(x);
        if (tmp_res.size < res.size)
        {
            res = tmp_res;
        }

        //find smallest level for Positive Davio
        move_to_bottom(x);
        change_expansion_type(x, expansion::PD);
        tmp_res = find_smallest_level(x);
        if (tmp_res.size < res.size)
        {
            res = tmp_res;
        }

        //find smallest level for Negative Davio
        move_to_bottom(x);
        change_expansion_type(x, expansion::ND);
        tmp_res = find_smallest_level(x);
        if (tmp_res.size < res.size)
        {
            res = tmp_res;
        }

        //move variable to smallest level with smallest expansion type
        move_to_bottom(x);

        change_expansion_type(x, res.exp);

        sift(var2lvl[x], res.pos);

        return res;
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        // choose the 0-leaf due to complemented edges in order to ensure canonicity
        auto const leaf = std::make_shared<detail::node<bool, bool>>(false);

        return std::array<edge_ptr, 2>{std::make_shared<detail::edge<bool, bool>>(false, leaf),
                                       std::make_shared<detail::edge<bool, bool>>(true, leaf)};
    }

    auto static transform(std::vector<kfdd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto sharpsat(edge_ptr const& f)
    {
        assert(f);

        if (f->v->is_const())
        {
            return f == consts[0] ? 0 : std::pow(2, var_count());
        }

        op::sharpsat op{f};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto count = (sharpsat(f->v->br().hi) + sharpsat(f->v->br().lo)) / 2;
        if (f->w)
        {  // complemented edge
            count = std::pow(2, var_count()) - count;
        }

        op.r = count;
        return cache(std::move(op))->r;
    }

    auto simplify(edge_ptr const& f, edge_ptr& g, edge_ptr& h) const noexcept
    {
        assert(f);
        assert(g);
        assert(h);

        if (f == g)
        {  // ite(f, f, h) => ite(f, 1, h)
            g = consts[1];
            return 1;
        }
        if (f == h)
        {  // ite(f, g, f) => ite(f, g, 0)
            h = consts[0];
            return 2;
        }
        if (f->v == h->v && !(f->w == h->w))
        {  // ite(f, g, ~f) => ite(f, g, 1)
            h = consts[1];
            return 3;
        }
        if (f->v == g->v && !(f->w == g->w))
        {  // ite(f, ~f, h) => ite(f, 0, h)
            g = consts[0];
            return 4;
        }
        return 0;
    }

    auto std_triple(std::int32_t const simpl, edge_ptr& f, edge_ptr& g, edge_ptr& h)
    {
        assert(f);
        assert(!f->v->is_const());
        assert(g);
        assert(h);

        switch (simpl)
        {
            case 1:
                assert(!h->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[h->v->br().x])
                {  // ite(f, 1, h) == ite(h, 1, f)
                    std::swap(f, h);
                }
                break;
            case 2:
                assert(!g->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[g->v->br().x])
                {  // ite(f, g, 0) == ite(g, f, 0)
                    std::swap(f, g);
                }
                break;
            case 3:
                assert(!g->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[g->v->br().x])
                {  // ite(f, g, 1) == ite(~g, ~f, 1)
                    f = complement(g);
                    g = complement(f);
                }
                break;
            case 4:
                assert(!h->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[h->v->br().x])
                {  // ite(f, 0, h) == ite(~h, 0, ~f)
                    f = complement(h);
                    h = complement(f);
                }
                break;
            default: assert(false);
        }
    }

    auto ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr override
    {
        assert(f);
        assert(g);
        assert(h);

        auto const ret = simplify(f, g, h);

        // terminal cases
        if (f == consts[0])
        {
            return h;
        }
        if (f == consts[1])
        {
            return g;
        }
        if (g == h)
        {
            return g;
        }
        if (h == consts[0] && g == consts[1])
        {
            return f;
        }
        if (g == consts[0] && h == consts[1])
        {
            return complement(f);
        }

        if (ret != 0)
        {
            std_triple(ret, f, g, h);
        }

        op::ite op{f, g, h};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = f->v->br().x == top_var(f, g) ? top_var(f, h) : top_var(g, h);

        op.r = make_branch(x, ite(cof(f, x, true), cof(g, x, true), cof(h, x, true)),
                           ite(cof(f, x, false), cof(g, x, false), cof(h, x, false)));
        return cache(std::move(op))->r;
    }

    // auto antiv(edge_ptr const& f, edge_ptr const& g)
    // {
    //     assert(f);
    //     assert(g);
    //
    //     if (f == consts[0])
    //     {
    //         return g;
    //     }
    //     if (g == consts[0])
    //     {
    //         return f;
    //     }
    //     if (f == consts[1])
    //     {
    //         return complement(g);
    //     }
    //     if (g == consts[1])
    //     {
    //         return complement(f);
    //     }
    //     if (f == g)
    //     {
    //         return consts[0];
    //     }
    //     if (f == complement(g))
    //     {
    //         return consts[1];
    //     }
    //
    //     op::antiv op{f, g};
    //     if (auto const* const ent = cached(op))
    //     {
    //         return ent->r;
    //     }
    //
    //     auto const x = top_var(f, g);
    //
    //     op.r = make_branch(x, antiv(cof(f, x, true), cof(g, x, true)), antiv(cof(f, x, false), cof(g, x, false)));
    //     return cache(std::move(op))->r;
    // }

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return antiv(f, g);
    }

    [[nodiscard]] auto agg(bool const& w, bool const& val) const noexcept -> bool override
    {
        return !(w == val);  // XOR
    }

    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const noexcept -> bool override
    {
        return !(w1 == w2);
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return !f->w ? uedge(true, f->v) : uedge(false, f->v);
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        //terminal cases
        // 0 * g = 0, f * 0 = 0
        if (f == consts[0] || g == consts[0])
        {
            return consts[0];
        }
        // 1 * g = g
        if (f == consts[1])
        {
            return g;
        }
        //f * 1 = f
        if (g == consts[1])
        {
            return f;
        }
        op::conj op{f, g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = top_var(f, g);
        auto f_low = cof(f, x, false);
        auto f_high = cof(f, x, true);

        auto g_low = cof(g, x, false);
        auto g_high = cof(g, x, true);
        edge_ptr high;
        edge_ptr low;
        switch (vl[x].t)
        {
            case expansion::S:
                high = conj(f_high, g_high);
                low = conj(f_low, g_low);
                break;
            case expansion::PD:
            case expansion::ND:
                high = antiv(conj(f_high, g_high), antiv(conj(f_low, g_high), conj(g_low, f_high)));
                low = conj(f_low, g_low);
                break;
            default: break;
        }

        op.r = make_branch(x, high, low);
        return cache(std::move(op))->r;
    }

    // auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    // {
    //     assert(f);
    //     assert(g);
    //
    //     if (f == consts[0] || g == consts[0])
    //     {  // something conjugated with 0 is 0
    //         return consts[0];
    //     }
    //     if (f == consts[1])
    //     {  // 1g == g
    //         return g;
    //     }
    //     if (g == consts[1])
    //     {  // f1 == f
    //         return f;
    //     }
    //     if (f->v == g->v)
    //     {  // check for complement
    //         return f->w == g->w ? f : consts[0];
    //     }
    //
    //     op::conj op{f, g};
    //     if (auto const* const ent = cached(op))
    //     {
    //         return ent->r;
    //     }
    //
    //     auto const x = top_var(f, g);
    //
    //     op.r = make_branch(x, conj(cof(f, x, true), cof(g, x, true)), conj(cof(f, x, false), cof(g, x, false)));
    //     return cache(std::move(op))->r;
    // }

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
        auto const t = vl[x].t;
        edge_ptr r;
        bool w;
        switch (t)
        {
            case expansion::S:
                if (hi == lo)  // redundancy rule
                {
                    return hi;  // without limitation of generality
                }

                w = lo->w;
                if (!w)
                {
                    //return uedge(w, unode(x, std::move(hi), std::move(lo)));
                    return uedge(w, unode(x, hi, lo));
                }
                return uedge(w, unode(x, complement(hi), complement(lo)));
                break;
            case expansion::PD:
            case expansion::ND:
                if (hi == consts[0])
                {
                    return lo;
                }
                if (lo->w)
                {
                    w = lo->w;
                    r = uedge(w, unode(x, hi, complement(lo)));
                }
                else
                {
                    r = uedge(false, unode(x, std::move(hi), std::move(lo)));
                }
                break;
            default: assert(false);
        }
        return r;
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
        return false;  // means a regular (non-complemented) edge
    }
};

auto inline kfdd::operator~() const
{
    assert(mgr);

    return kfdd{mgr->complement(f), mgr};
}

auto inline kfdd::operator&=(kfdd const& rhs) -> kfdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline kfdd::operator|=(kfdd const& rhs) -> kfdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline kfdd::operator^=(kfdd const& rhs) -> kfdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline kfdd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

auto inline kfdd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

auto inline kfdd::high() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return kfdd{f->v->br().hi, mgr};
}

auto inline kfdd::low() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return kfdd{f->v->br().lo, mgr};
}

template <typename T, typename... Ts>
auto inline kfdd::fn(T const a, Ts... args) const
{
    assert(mgr);

    return kfdd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline kfdd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline kfdd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline kfdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline kfdd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline kfdd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline kfdd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline kfdd::ite(kfdd const& g, kfdd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);  // transitive property

    return kfdd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline kfdd::compose(std::int32_t const x, kfdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return kfdd{mgr->compose(f, x, g.f), mgr};
}

auto inline kfdd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return kfdd{mgr->restr(f, x, a), mgr};
}

auto inline kfdd::exist(std::int32_t const x) const
{
    assert(mgr);

    return kfdd{mgr->exist(f, x), mgr};
}

auto inline kfdd::forall(std::int32_t const x) const
{
    assert(mgr);

    return kfdd{mgr->forall(f, x), mgr};
}

auto inline kfdd::sharpsat() const
{
    assert(mgr);

    return mgr->sharpsat(f);
}

auto inline kfdd::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
