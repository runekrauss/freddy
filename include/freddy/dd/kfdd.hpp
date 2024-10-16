#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager

#include <algorithm>  // std::transform
#include <array>      // std::array
#include <cassert>    // assert
#include <cmath>      // std::pow
#include <cstdint>    // std::int32_t
#include <iostream>   // std::cout
#include <iterator>   // std::back_inserter
#include <memory>     // std::shared_ptr
#include <ostream>    // std::ostream
#include <string>     // std::string
#include <utility>    // std::make_pair
#include <vector>     // std::vector

#include "freddy/op/sharpsat.hpp"
#include "freddy/op/conj.hpp"
#include "freddy/op/antiv.hpp"
#include "freddy/op/ite.hpp"

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

class kfdd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class kfdd
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

        return (lhs.f == rhs.f);
    }

    auto friend operator!=(kfdd const& lhs, kfdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, kfdd const& g) -> std::ostream&
    {
        s << "Wrapper = " << g.f;
        s << "\nkfdd manager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(kfdd const& g) const noexcept
    {
        assert(f);

        return (f->v == g.f->v);
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
    auto cof(T, Ts...) const;

    [[nodiscard]] auto size() const -> size_t;

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

    void print() const;

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
            manager(tmls())
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

    std::string e_to_s(expansion e)
    {
        switch(e)
        {
            case expansion::S:
                return "S";
            break;
            case expansion::PD:
                return "PD";
            break;
            case expansion::ND:
                return "ND";
            break;
            default: assert(false); break;
        }
    }

    void dtl_sift(kfdd f)
    {
        assert(f.mgr == this);
        //run through all variables
        auto start_size = f.size();
        //for each variable:
        //move to first position
        //move to last position, recording minimum size
        //change type, sift to first position, recording minimum size
        //change type, sift to last position, recording minimum size
        //change type to minimum type and move to minimum position
        //CURRENTLY WORKING DIFFERENTLY FOR TESTING
        for(auto x = 0; x < var_count();x++)
        {
            auto const initial_size = f.size();
            auto min_size = f.size();
            auto min_pos = 0;
            auto min_t = vl[x].t;
            std::cout << "Start sifting for variable " << x << "\n";
            std::cout << "t: " << e_to_s(vl[x].t) << ", pos: " << var2lvl[x] << ", size: " << min_size << "\n";
            //move to first position
            change_expansion_type(x, expansion::S);
            sift(var2lvl[x], 0);
            size_t size = f.size();
            size_t old_size = SIZE_MAX;
            if(size < min_size)
            {
                min_pos = 0;
                min_t = expansion::S;
                std::cout << "Found a new min size of " << size << " (was " << min_size << ") with variable " << x << " at position " << min_pos << " and expansion type S\n";
                min_size = size;
            }
            std::cout << "Moved to pos 0" << "\n";
            std::cout << "t: " << e_to_s(vl[x].t) << ", size: " << size << "\n";
            for(auto i = 0; i < var_count() - 1; i++)
            {
                old_size = f.size();
                sift(i, i+1);
                size = f.size();
                std::cout << "Sift: var " << x << ", pos " << std::format("{:03}", i) << " => " << std::format("{:03}", i+1)  << ", t:  S" << ", size: " << old_size << " => " << size << "\n";
                if(size < min_size)
                {
                    min_pos = i+1;
                    min_t = expansion::S;
                    std::cout << "Found a new min size of " << size << " (was " << min_size << ") with variable " << x << " at position " << min_pos << " and expansion type S\n";
                    min_size = size;
                }
            }
            change_expansion_type(x, expansion::PD);
            sift(var2lvl[x], 0);
                size = f.size();
            if(size < min_size)
            {
                min_pos = 0;
                min_t = expansion::PD;
                std::cout << "Found a new min size of " << size << " (was " << min_size << ") with variable " << x << " at position " << min_pos << " and expansion type PD\n";
                min_size = size;
            }
            for(auto i = 0; i < var_count() - 1; i++)
            {
                old_size = f.size();
                sift(i, i+1);
                size = f.size();
                std::cout << "Sift: var " << x << ", pos " << std::format("{:03}", i) << " => " << std::format("{:03}", i+1)  << ", t: PD" << ", size: " << old_size << " => " << size << "\n";
                if(size < min_size)
                {
                    min_pos = i+1;
                    min_t = expansion::PD;
                    std::cout << "Found a new min size of " << size << " (was " << min_size << ") with variable " << x << " at position " << min_pos << " and expansion type PD\n";
                    min_size = size;
                }
            }
            change_expansion_type(x, expansion::ND);
            sift(var2lvl[x], 0);
                size = f.size();
            if(size < min_size)
            {
                min_pos = 0;
                min_t = expansion::ND;
                std::cout << "Found a new min size of " << size << " (was " << min_size << ") with variable " << x << " at position " << min_pos << " and expansion type ND\n";
                min_size = size;
            }
            for(auto i = 0; i < var_count() - 1; i++)
            {
                old_size = f.size();
                sift(i, i+1);
                size = f.size();
                std::cout << "Sift: var " << x << ", pos " << std::format("{:03}", i) << " => " << std::format("{:03}", i+1)  << ", t: ND" << ", size: " << old_size << " => " << size << "\n";
                if(size < min_size)
                {
                    min_pos = i+1;
                    min_t = expansion::ND;
                    std::cout << "Found a new min size of " << size << " (was " << min_size << ") with variable " << x << " at position " << min_pos << " and expansion type ND\n";
                    min_size = size;
                }
            }
            change_expansion_type(x, min_t);
            sift(var_count()-1, min_pos);
            std::cout << "Sifting for variable " << x << " done. Minimal pos at " << min_pos << " with expansion type " << e_to_s(min_t) << " old size: " << initial_size << " new size: " << f.size() <<  "\n";
        }
        std::cout << "dtl sifting done" << "\n";
        std::cout << "old size: " << start_size << ", new size: " << f.size() << "\n";
        for(auto x = 0; x < var_count();x++)
        {
            std::cout << "var: " << x << ", t: " << e_to_s(vl[x].t) << ", pos: " << var2lvl[x] << "\n";
        }
    }

    void change_expansion_type(int x, freddy::expansion t)
    {
        assert(x>=0);
        assert(x < var_count());
        auto var_t = vl[x].t;
        if(var_t==t)
        {
            //type doesn't change, nothing to do
            return;
        }
        vl[x].t = t;
        if(var_t == expansion::S)
        {
            //S => PD
            if(t == expansion::PD)
            {
                for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();++it)
                {
                    auto node = *it;
                    node->br().hi = antiv(node->br().hi,node->br().lo);
                }
            }
            else if(t == expansion::ND)
            {
                for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();++it)
                {
                    auto node = *it;
                    auto low = node->br().lo;
                    auto high = node->br().hi;
                    node->br().hi = low;
                    node->br().lo = high;
                    node->br().hi = antiv(node->br().hi,node->br().lo);
                }
            }
        }
        else if(var_t == expansion::PD)
        {
            if(t == expansion::S)
            {
                for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();++it)
                {
                    auto node = *it;
                    node->br().hi = antiv(node->br().hi,node->br().lo);
                }
            }
            else if(t == expansion::ND)
            {
                for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();++it)
                {
                    auto node = *it;
                    node->br().lo = antiv(node->br().hi,node->br().lo);
                }
            }
        }
        else if(var_t == expansion::ND)
        {
            if(t == expansion::S)
            {
                for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();++it)
                {
                    auto node = *it;
                    auto low = node->br().lo;
                    auto high = node->br().hi;
                    node->br().hi = low;
                    node->br().lo = high;
                    node->br().lo = antiv(node->br().hi,node->br().lo);
                }
            }
            else if(t == expansion::PD)
            {
                for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();++it)
                {
                    auto node = *it;
                    node->br().lo = antiv(node->br().hi,node->br().lo);
                }
            }
        }
        gc();
    }

  private:
    using bool_edge = detail::edge<bool, bool>;

    using bool_node = detail::node<bool, bool>;

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
        op::ite op{f,g,h};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = (f->v->br().x == top_var(f, g)) ? top_var(f, h) : top_var(g, h);
        op.r = make_branch(x, ite(cof(f, x, true), cof(g, x, true), cof(h, x, true)),
                             ite(cof(f, x, false), cof(g, x, false), cof(h, x, false)));

        return cache(std::move(op))->r;
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g) -> edge_ptr
    {
        assert(f);
        assert(g);
        // terminal cases
        if (f == consts[0])
        {
            return g;
        }

        if (g == consts[0])
        {
            return f;
        }

        if (f == g)
        {
            return consts[0];
        }
        op::antiv op{f,g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = top_var(f, g);

        auto f_low = cof(f, x, false);
        auto f_high = cof(f, x, true);

        auto g_low = cof(g, x, false);
        auto g_high = cof(g, x, true);

        auto low = antiv(f_low, g_low);
        auto high = antiv(f_high, g_high);


        op.r = make_branch(x, high, low);
        return cache(std::move(op))->r;
    }

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
        return !(w1 == w2);  // XOR
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);
        return foa(std::make_shared<bool_edge>(merge(f->w, true), f->v));
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
        op::conj op{f,g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }
        //cr = ct.find({operation::AND, g, f});
        //if (cr != ct.end())
        //{
        //    return cr->second.first.lock();
        //}

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
                    return foa(std::make_shared<bool_edge>(
                        w, foa(std::make_shared<bool_node>(x, std::move(hi), std::move(lo)))));
                }
                return foa(std::make_shared<bool_edge>(
                    w, foa(std::make_shared<bool_node>(x, complement(hi), complement(lo)))));
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
                    r = foa(std::make_shared<bool_edge>(w, foa(std::make_shared<bool_node>(x, hi, complement(lo)))));
                }
                else
                {
                    r = foa(std::make_shared<bool_edge>(
                        false, foa(std::make_shared<bool_node>(x, std::move(hi), std::move(lo)))));
                }
                break;
            default: assert(false);
        }
        return r;
    }

    // auto make_canonical(edge_ptr f) -> edge_ptr
    // {
    //     return f;
    //     assert(f);
    //     //terminals are per definition canonical
    //     if (f->v->is_const())
    //     {
    //         return f;
    //     }
    //     auto const br = f->v->br();
    //     if (!f->w)
    //     {
    //         return f;
    //     }
    //     auto comp = br.lo;
    //     if(br.lo==consts[0])
    //     {
    //         comp = consts[1];
    //     }
    //     else if(br.lo==consts[1])
    //     {
    //         comp = consts[0];
    //     }
    //     else
    //     {
    //         comp = complement(br.lo);
    //     }
    //     auto canon = foa(std::make_shared<bool_edge>(false, foa(std::make_shared<bool_node>(br.x, br.hi,
    //     make_canonical(comp))))); return canon;
    // }

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

    auto static transform(std::vector<kfdd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto sharpsat(edge_ptr const& f) -> double
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

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        // choose the 0-leaf due to complemented edges in order to ensure canonicity
        auto const leaf = std::make_shared<bool_node>(false);

        return std::array<edge_ptr, 2>{std::make_shared<bool_edge>(false, leaf),
                                       std::make_shared<bool_edge>(true, leaf)};
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

    return (*this == mgr->zero());
}

auto inline kfdd::is_one() const noexcept
{
    assert(mgr);

    return (*this == mgr->one());
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
auto inline kfdd::cof(T const a, Ts... args) const
{
    assert(mgr);

    return kfdd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

size_t inline kfdd::size() const
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


void inline kfdd::print() const
{
    assert(mgr);

    mgr->print({*this});
}

}  // namespace freddy::dd
