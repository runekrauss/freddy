#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "eddy/detail/manager.hpp"  // detail::manager

#include <algorithm>    // std::transform
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

namespace eddy::dd
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
    bdd() = default;  // so that it initially works with standard containers

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
        s << "Wrapper: " << g.f;
        s << "\nManager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto is_complemented() const noexcept
    {
        assert(f);

        return (f->w != 0);
    }

    [[nodiscard]] auto equals(bdd const& g) const noexcept
    {
        assert(f);

        return (f->v == g.f->v);
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        assert(f);

        return !f->v;
    }

    [[nodiscard]] auto var() const noexcept
    {
        assert(!is_const());

        return f->v->x;
    }

    [[nodiscard]] auto high(bool = false) const;

    [[nodiscard]] auto low(bool = false) const;

    template <typename T, typename... Ts>
    auto cof(T, Ts...) const;

    template <typename T, typename... Ts>
    auto eval(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto is_essential(std::int32_t) const noexcept;

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto ite(bdd const&, bdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    [[nodiscard]] auto satcount() const;

    auto print() const;

  private:
    // wrapper is controlled by its BDD manager
    bdd(std::shared_ptr<detail::edge> f, bdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge> f;  // DD handle

    bdd_manager* mgr{};  // must be destroyed after this wrapper

    friend bdd_manager;
};

class bdd_manager : public detail::manager
{
  public:
    auto var(std::string_view l = {})
    {
        return bdd{make_var(detail::decomposition::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bdd{vars[i], this};
    }

    auto zero() noexcept
    {
        return bdd{tmls[0], this};
    }

    auto one() noexcept
    {
        return bdd{tmls[1], this};
    }

    auto size(std::vector<bdd> const& fs)
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bdd> const& fs) const
    {
        return longest_path(transform(fs));
    }

    auto print(std::vector<bdd> const& fs, std::vector<std::string> outputs = {}, std::ostream& s = std::cout)
    {
        to_dot(transform(fs), std::move(outputs), s);
    }

  private:
    [[nodiscard]] static auto transform(std::vector<bdd> const& fs) -> std::vector<std::shared_ptr<detail::edge>>
    {
        std::vector<std::shared_ptr<detail::edge>> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto satcount_rec(std::shared_ptr<detail::edge> const& f) -> double  // as results can be very large
    {
        assert(f);

        if (!f->v)
        {
            return (f == tmls[0]) ? 0 : std::pow(2, var_count());
        }

        auto const cr = ct.find({operation::SAT, f});
        if (cr != ct.end())
        {
            return cr->second.second;
        }

        auto count = (satcount_rec(f->v->hi) + satcount_rec(f->v->lo)) / 2;
        if (f->w != 0)
        {  // complemented edge
            count = std::pow(2, var_count()) - count;
        }

        ct.insert_or_assign({operation::SAT, f}, std::make_pair(std::shared_ptr<detail::edge>{}, count));

        return count;
    }

    auto satcount(std::shared_ptr<detail::edge> const& f)
    {
        assert(f);

        if (f == tmls[0])
        {
            return 0.0;
        }
        if (f == tmls[1])
        {
            return std::pow(2, var_count());
        }

        return satcount_rec(f);
    }

    auto simplify(std::shared_ptr<detail::edge>& f, std::shared_ptr<detail::edge>& g,
                  std::shared_ptr<detail::edge>& h) const noexcept
    {
        assert(f);
        assert(g);
        assert(h);

        if (f == g)
        {  // ite(f, f, h) => ite(f, 1, h)
            g = tmls[1];
            return 1;
        }
        if (f == h)
        {  // ite(f, g, f) => ite(f, g, 0)
            h = tmls[0];
            return 2;
        }
        if (f->v == h->v && ((f->w == 0 && h->w == 1) || (f->w == 1 && h->w == 0)))
        {  // ite(f, g, !f) => ite(f, g, 1)
            h = tmls[1];
            return 3;
        }
        if (f->v == g->v && ((f->w == 0 && g->w == 1) || (f->w == 1 && g->w == 0)))
        {  // ite(f, !f, h) => ite(f, 0, h)
            g = tmls[0];
            return 4;
        }
        return 0;
    }

    auto std_triple(std::int32_t const simplification, std::shared_ptr<detail::edge>& f,
                    std::shared_ptr<detail::edge>& g, std::shared_ptr<detail::edge>& h)
    {
        assert(f);
        assert(g);
        assert(h);

        switch (simplification)
        {
            case 1:
                assert(f->v);
                assert(h->v);

                if (var2lvl[f->v->x] >= var2lvl[h->v->x])
                {  // ite(f, 1, h) = ite(h, 1, f)
                    std::swap(f, h);
                }
                break;
            case 2:
                assert(f->v);
                assert(g->v);

                if (var2lvl[f->v->x] >= var2lvl[g->v->x])
                {  // ite(f, g, 0) = ite(g, f, 0)
                    std::swap(f, g);
                }
                break;
            case 3:
                assert(f->v);
                assert(g->v);

                if (var2lvl[f->v->x] >= var2lvl[g->v->x])
                {  // ite(f, g, 1) = ite(!g, !f, 1)
                    f = complement(g);
                    g = complement(f);
                }
                break;
            case 4:
                assert(f->v);
                assert(h->v);

                if (var2lvl[f->v->x] >= var2lvl[h->v->x])
                {  // ite(f, 0, h) = ite(!h, 0, !f)
                    f = complement(h);
                    h = complement(f);
                }
                break;
            default: assert(false);
        }
    }

    auto ite(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g, std::shared_ptr<detail::edge> h)
    {
        assert(f);
        assert(g);
        assert(h);

        auto const ret = simplify(f, g, h);

        // terminal cases
        if (f == tmls[1])
        {
            return g;
        }
        if (f == tmls[0])
        {
            return h;
        }
        if (g == h)
        {
            return g;
        }
        if (g == tmls[1] && h == tmls[0])
        {
            return f;
        }
        if (g == tmls[0] && h == tmls[1])
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

        auto const x = (f->v->x == top_var(f, g)) ? top_var(f, h) : top_var(g, h);
        auto r = make_branch(x, ite(cof(f, x, true), cof(g, x, true), cof(h, x, true)),
                             ite(cof(f, x, false), cof(g, x, false), cof(h, x, false)));

        ct.insert_or_assign({operation::ITE, f, g, h}, std::make_pair(r, 0.0));

        return r;
    }

    auto antiv(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g)
    {
        assert(f);
        assert(g);

        if (f == tmls[0])
        {
            return g;
        }
        if (g == tmls[0])
        {
            return f;
        }
        if (f == tmls[1])
        {
            return complement(g);
        }
        if (g == tmls[1])
        {
            return complement(f);
        }
        if (f == g)
        {
            return tmls[0];
        }
        if (f == complement(g))
        {
            return tmls[1];
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

    auto add(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g) -> std::shared_ptr<detail::edge> override
    {
        assert(f);
        assert(g);

        return antiv(f, g);
    }

    auto apply(std::int32_t const w, std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {
        assert(f);

        return foa(std::make_shared<detail::edge>(((f->w == 1 && w == 0) || (f->w == 0 && w == 1)) ? 1 : 0, f->v));
    }

    auto complement(std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {
        assert(f);

        return ((f->w == 0) ? foa(std::make_shared<detail::edge>(1, f->v))
                            : foa(std::make_shared<detail::edge>(0, f->v)));
    }

    auto conj(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g)
        -> std::shared_ptr<detail::edge> override
    {
        assert(f);
        assert(g);

        if (f == tmls[1])
        {  // 1g = g
            return g;
        }
        if (g == tmls[1])
        {  // f1 = f
            return f;
        }
        if (f->v == g->v)
        {  // check for complement
            return ((f->w == g->w) ? f : tmls[0]);
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

    auto disj(std::shared_ptr<detail::edge> const& f, std::shared_ptr<detail::edge> const& g)
        -> std::shared_ptr<detail::edge> override
    {
        assert(f);
        assert(g);

        return complement(conj(complement(f), complement(g)));
    }

    [[nodiscard]] auto is_normalized([[maybe_unused]] std::shared_ptr<detail::edge> const& hi,
                                     std::shared_ptr<detail::edge> const& lo) const noexcept -> bool override
    {
        assert(hi);
        assert(lo);

        return (lo->w != 0);
    }

    auto make_branch(std::int32_t const x, std::shared_ptr<detail::edge> hi, std::shared_ptr<detail::edge> lo)
        -> std::shared_ptr<detail::edge> override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == lo)  // redundancy rule
        {
            return hi;  // without limitation of generality
        }

        auto const w = is_normalized(hi, lo) ? 1 : 0;
        return foa(std::make_shared<detail::edge>(
            w, foa(std::make_shared<detail::node>(x, (w == 0) ? std::move(hi) : complement(hi),
                                                  (w == 0) ? std::move(lo) : complement(lo)))));
    }

    auto mul(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g) -> std::shared_ptr<detail::edge> override
    {
        assert(f);
        assert(g);

        return conj(f, g);
    }

    auto neg(std::shared_ptr<detail::edge> const& f) noexcept -> std::shared_ptr<detail::edge> override
    {
        assert(f);

        return f;
    }

    [[nodiscard]] auto regw() const noexcept -> std::int32_t override
    {
        return 0;  // means a regular (non-complemented) edge
    }

    friend bdd;
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

template <typename T, typename... Ts>
auto inline bdd::eval(T const a, Ts... args) const
{
    assert(mgr);

    return mgr->eval(f, a, std::forward<Ts>(args)...);
}

auto inline bdd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bdd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bdd::is_essential(std::int32_t const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
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

}  // namespace eddy::dd
