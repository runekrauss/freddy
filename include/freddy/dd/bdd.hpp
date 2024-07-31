#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager

#include <algorithm>    // std::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::pow
#include <cstdint>      // std::int32_t
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
#include <memory>       // std::shared_ptr
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::make_pair
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

class bdd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class bdd
{
  public:
    bdd() = default;  // so that BDDs initially work with standard containers

    auto operator~() const;

    auto operator&=(bdd const&) -> bdd&;

    auto operator|=(bdd const&) -> bdd&;

    auto operator^=(bdd const&) -> bdd&;

    auto friend operator&(bdd lhs, bdd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(bdd lhs, bdd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(bdd lhs, bdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(bdd const& lhs, bdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BDD manager

        return (lhs.f == rhs.f);
    }

    auto friend operator!=(bdd const& lhs, bdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bdd const& g) -> std::ostream&
    {
        s << "Wrapper = " << g.f;
        s << "\nBDD manager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto equals(bdd const& g) const noexcept
    {  // check if edges point to the same node
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

    [[nodiscard]] auto high(bool = false) const;

    [[nodiscard]] auto low(bool = false) const;

    template <typename T, typename... Ts>
    auto cof(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto has_const(bool) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(bdd const&, bdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    [[nodiscard]] auto satcount() const;

    auto print() const;

  private:
    friend bdd_manager;

    // wrapper is controlled by its BDD manager
    bdd(std::shared_ptr<detail::edge<bool, bool>> f, bdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, bool>> f;  // DD handle

    bdd_manager* mgr{};  // must be destroyed after this wrapper
};

class bdd_manager : public detail::manager<bool, bool>
{
  public:
    friend bdd;

    bdd_manager() :
            manager(tmls())
    {}

    auto var(std::string_view l = {})
    {
        return bdd{make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bdd{vars[i], this};
    }

    auto zero() noexcept
    {
        return bdd{consts[0], this};
    }

    auto one() noexcept
    {
        return bdd{consts[1], this};
    }

    [[nodiscard]] auto size(std::vector<bdd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bdd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto print(std::vector<bdd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        to_dot(transform(fs), outputs, s);
    }

  private:
    using bool_edge = detail::edge<bool, bool>;

    using bool_node = detail::node<bool, bool>;

    auto satcount(edge_ptr const& f) -> double
    {
        assert(f);

        if (f->v->is_const())
        {
            return (f == consts[0]) ? 0 : std::pow(2, var_count());
        }

        auto const cr = ct.find({operation::SAT, f});
        if (cr != ct.end())
        {
            return cr->second.second;
        }

        auto count = (satcount(f->v->br().hi) + satcount(f->v->br().lo)) / 2;
        if (f->w)
        {  // complemented edge
            count = std::pow(2, var_count()) - count;
        }

        ct.insert_or_assign({operation::SAT, f}, std::make_pair(edge_ptr{}, count));

        return count;
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

        auto const cr = ct.find({operation::ITE, f, g, h});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto const x = (f->v->br().x == top_var(f, g)) ? top_var(f, h) : top_var(g, h);
        auto r = make_branch(x, ite(cof(f, x, true), cof(g, x, true), cof(h, x, true)),
                             ite(cof(f, x, false), cof(g, x, false), cof(h, x, false)));

        ct.insert_or_assign({operation::ITE, f, g, h}, std::make_pair(r, 0.0));

        return r;
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        if (f == consts[0])
        {
            return g;
        }
        if (g == consts[0])
        {
            return f;
        }
        if (f == consts[1])
        {
            return complement(g);
        }
        if (g == consts[1])
        {
            return complement(f);
        }
        if (f == g)
        {
            return consts[0];
        }
        if (f == complement(g))
        {
            return consts[1];
        }

        auto const cr = ct.find({operation::XOR, f, g});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto const x = top_var(f, g);
        auto r = make_branch(x, antiv(cof(f, x, true), cof(g, x, true)), antiv(cof(f, x, false), cof(g, x, false)));

        ct.insert_or_assign({operation::XOR, f, g}, std::make_pair(r, 0.0));

        return r;
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
        return !(w1 == w2);
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return (!f->w ? foa(std::make_shared<bool_edge>(true, f->v)) : foa(std::make_shared<bool_edge>(false, f->v)));
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == consts[1])
        {  // 1g == g
            return g;
        }
        if (g == consts[1])
        {  // f1 == f
            return f;
        }
        if (f->v == g->v)
        {  // check for complement
            return ((f->w == g->w) ? f : consts[0]);
        }

        auto const cr = ct.find({operation::AND, f, g});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto const x = top_var(f, g);
        auto r = make_branch(x, conj(cof(f, x, true), cof(g, x, true)), conj(cof(f, x, false), cof(g, x, false)));

        ct.insert_or_assign({operation::AND, f, g}, std::make_pair(r, 0.0));

        return r;
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

        if (hi == lo)  // redundancy rule
        {
            return hi;  // without limitation of generality
        }

        return foa(std::make_shared<bool_edge>(
            lo->w, foa(std::make_shared<bool_node>(x, !lo->w ? std::move(hi) : complement(hi),
                                                   !lo->w ? std::move(lo) : complement(lo)))));
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

    auto static transform(std::vector<bdd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        // choose the 0-leaf due to complemented edges in order to ensure canonicity
        auto const leaf = std::make_shared<bool_node>(false);

        return std::array<edge_ptr, 2>{std::make_shared<bool_edge>(false, leaf),
                                       std::make_shared<bool_edge>(true, leaf)};
    }
};

auto inline bdd::operator~() const
{
    assert(mgr);

    return bdd{mgr->complement(f), mgr};
}

auto inline bdd::operator&=(bdd const& rhs) -> bdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);
    std::cout << "TEST6" << std::endl;
    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline bdd::operator|=(bdd const& rhs) -> bdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline bdd::operator^=(bdd const& rhs) -> bdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline bdd::is_zero() const noexcept
{
    assert(mgr);

    return (*this == mgr->zero());
}

auto inline bdd::is_one() const noexcept
{
    assert(mgr);

    return (*this == mgr->one());
}

auto inline bdd::high(bool const weighting) const
{
    assert(mgr);

    return bdd{mgr->high(f, weighting), mgr};
}

auto inline bdd::low(bool const weighting) const
{
    assert(mgr);

    return bdd{mgr->low(f, weighting), mgr};
}

template <typename T, typename... Ts>
auto inline bdd::cof(T const a, Ts... args) const
{
    assert(mgr);

    return bdd{mgr->subfunc(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline bdd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bdd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bdd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline bdd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline bdd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline bdd::ite(bdd const& g, bdd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);  // transitive property

    return bdd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline bdd::compose(std::int32_t const x, bdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bdd{mgr->compose(f, x, g.f), mgr};
}

auto inline bdd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bdd{mgr->restr(f, x, a), mgr};
}

auto inline bdd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bdd{mgr->exist(f, x), mgr};
}

auto inline bdd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bdd{mgr->forall(f, x), mgr};
}

auto inline bdd::satcount() const
{
    assert(mgr);

    return mgr->satcount(f);
}

auto inline bdd::print() const
{
    assert(mgr);

    mgr->print({*this});
}

}  // namespace freddy::dd
