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
#include <numeric>      // std::gcd
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::make_pair
#include <vector>       // std::vector
#include <bit>          // std::bit_width

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

class phdd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class phdd
{
  public:
    phdd() = default;  // so that phdds initially work with standard containers

    auto operator+=(phdd const&) -> phdd&;

    auto operator-=(phdd const&) -> phdd&;

    auto operator*=(phdd const&) -> phdd&;

    auto operator-() const;

    auto operator~() const;

    auto operator&=(phdd const&) -> phdd&;

    auto operator|=(phdd const&) -> phdd&;

    auto operator^=(phdd const&) -> phdd&;

    auto friend operator+(phdd lhs, phdd const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    auto friend operator-(phdd lhs, phdd const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    auto friend operator*(phdd lhs, phdd const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    auto friend operator&(phdd lhs, phdd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(phdd lhs, phdd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(phdd lhs, phdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(phdd const& lhs, phdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same phdd manager

        return (lhs.f == rhs.f);
    }

    auto friend operator!=(phdd const& lhs, phdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, phdd const& g) -> std::ostream&
    {
        s << "Wrapper = " << g.f;
        s << "\nphdd manager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(phdd const& g) const noexcept
    {
        assert(f);

        return (f->v == g.f->v);
    }

    [[nodiscard]] auto weight() const noexcept
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

    [[nodiscard]] auto is_two() const noexcept;

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

    [[nodiscard]] auto has_const(std::int32_t) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(phdd const&, phdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, phdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print() const;

  private:
    friend phdd_manager;

    // wrapper is controlled by its phdd manager
    phdd(std::shared_ptr<detail::edge<std::int32_t, std::int32_t>> f, phdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<std::int32_t, std::int32_t>> f;

    phdd_manager* mgr{};
};

class phdd_manager : public detail::manager<std::int32_t, std::int32_t>
{
  public:
    friend phdd;

    phdd_manager() :
            manager(tmls())
    {
        consts.push_back(make_const(1, 1));
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        auto const zero_leaf = std::make_shared<int_node>(0);
        auto const one_leaf = std::make_shared<int_node>(1);

        return std::array<edge_ptr, 2>{std::make_shared<int_edge>(0, zero_leaf),
                                       std::make_shared<int_edge>(0, one_leaf)};
    }

    auto var(std::string_view l = {})
    {
        return phdd{make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return phdd{vars[i], this};
    }

    auto zero() noexcept
    {
        return phdd{consts[0], this};
    }

    auto one() noexcept
    {
        return phdd{consts[1], this};
    }

    auto two() noexcept
    {
        return phdd{consts[2], this};
    }

    auto static factorize_by_pow2(int w) -> std::pair<int,int>
    {
        const unsigned int pow = w & (-w);
        auto exp = std::bit_width(pow) - 1;
        return {exp, w / pow};
    }

    auto constant(std::int32_t const w)
    {
        if(w == 0) { return zero();}

        auto factors = factorize_by_pow2(w);
        return phdd{make_const(factors.first, factors.second), this};
    }

    [[nodiscard]] auto size(std::vector<phdd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<phdd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto weighted_sum(std::vector<phdd> const& fs)
    {
        auto r = consts[0];
        for (auto i = 0; i < static_cast<std::int32_t>(fs.size()); ++i)
        {  // LSB ... MSB
            r = add(r, mul(make_const(static_cast<std::int32_t>(std::pow(2, i)), 1), fs[i].f));
        }
        return phdd{r, this};
    }

    auto twos_complement(std::vector<phdd> const& fs)
    {
        assert(!fs.empty());

        return phdd{add(apply(static_cast<std::int32_t>(-std::pow(2, fs.size() - 1)), fs.back().f),
                       weighted_sum({fs.begin(), fs.end() - 1}).f),
                   this};
    }

    auto print(std::vector<phdd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        to_dot(transform(fs), outputs, s);
    }

  private:
    using int_edge = detail::edge<std::int32_t, std::int32_t>;

    using int_node = detail::node<std::int32_t, std::int32_t>;

    [[deprecated]] auto neg(edge_ptr const& f)
    {
        assert(f);

        return ((f == consts[0]) ? f : mul(consts[3], f));
    }

    [[deprecated]] auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return add(f, neg(g));
    }

    [[deprecated]] auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return sub(add(f, g), mul(consts[2], mul(f, g)));
    }

    [[deprecated]] auto rearrange(operation const op, edge_ptr& f, edge_ptr& g)
    {  // increase the probability of reusing previously computed results
        assert(f);
        assert(g);

        std::int32_t w = 0;
        switch (op)
        {
            case operation::ADD:
                if (std::abs(f->w) <= std::abs(g->w))
                {
                    std::swap(f, g);
                    w = normw(f, g);
                }
                else
                {
                    w = normw(g, f);
                }

                f = foa(std::make_shared<int_edge>(f->w - w, f->v));
                g = foa(std::make_shared<int_edge>(g->w - w, g->v));
                break;
            case operation::MUL:
                w = f->w + g->w;

                if (f->v->operator()() <= g->v->operator()())
                {
                    std::swap(f, g);
                }

                f = foa(std::make_shared<int_edge>(0, f->v));
                g = foa(std::make_shared<int_edge>(0, g->v));
                break;
            default: assert(false);
        }
        return w;
    }

    [[deprecated]] auto apply(std::int32_t const& w, edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        if (w == 0)
        {
            return f;
        }

        return foa(std::make_shared<int_edge>(comb(w, f->w), f->v));
    }

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
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
        if (f->v->is_const() && g->v->is_const())
        {
            // TODO mind negative edge weights
            auto val = agg(f->w,f->v->c()) + agg(g->w,g->v->c());
            auto factors = factorize_by_pow2(val);
            return make_const(factors.first, factors.second);
        }

        auto const w = rearrange(operation::ADD, f, g);

        auto const cr = ct.find({operation::ADD, f, g});
        if (cr != ct.end())
        {
            return apply(w, cr->second.first.lock());
        }

        auto const x = top_var(f, g);
        auto const r = make_branch(x, add(cof(f, x, true), cof(g, x, true)), add(cof(f, x, false), cof(g, x, false)));

        ct.insert_or_assign({operation::ADD, f, g}, std::make_pair(r, 0.0));

        return apply(w, r);
    }

    [[deprecated,nodiscard]] auto agg(std::int32_t const& w, std::int32_t const& val) const noexcept -> std::int32_t override
    {
        return ((1 << w) * val);
    }

    // TODO only used in apply!!
    [[deprecated,nodiscard]] auto comb(std::int32_t const& w1, std::int32_t const& w2) const noexcept -> std::int32_t override
    {
        return (w1 + w2);
    }

    [[deprecated]] auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return sub(consts[1], f);
    }

    [[deprecated]] auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return mul(f, g);
    }

    [[deprecated]] auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
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
        return sub(add(f, g), mul(f, g));
    }

    [[deprecated]] auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);
        assert(hi->v != lo->v);

        // TODO if-condition x is Shannon decomposed elif PD else

        if(hi == consts[0])
        {
            return foa(std::make_shared<int_edge>(
                lo->w,
                foa(std::make_shared<int_node>(x, hi, foa(std::make_shared<int_edge>(0, lo->v))))));
        }
        if(lo == consts[0])
        {
            return foa(std::make_shared<int_edge>(
                hi->w,
                foa(std::make_shared<int_node>(x, foa(std::make_shared<int_edge>(0, hi->v)), lo))));
        }
        auto const w = normw(hi, lo);

        return foa(std::make_shared<int_edge>(
            w,
            foa(std::make_shared<int_node>(x,
                                           foa(std::make_shared<int_edge>(hi->w - w, hi->v)),
                                           foa(std::make_shared<int_edge>(lo->w - w, lo->v))))));
    }

    [[deprecated, nodiscard]] auto merge(std::int32_t const& val1, std::int32_t const& val2) const noexcept -> std::int32_t override
    {
        return (val1 + val2);
    }

    [[deprecated]] auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        // TODO switch for decomposition types

        if (f == consts[0] || g == consts[0])
        {
            return consts[0];
        }
        if (f->v == consts[1]->v)
        {
            return apply(f->w, g);
        }
        if (f->v == consts[1]->v)
        {
            return apply(g->w, f);
        }
        if (f->v->is_const() && g->v->is_const())
        {
            // TODO negative edge weights
            return this->constant(agg(f->w,f->v->c()) * agg(g->w, g->v->c())).f;
        }

        auto const w = rearrange(operation::MUL, f, g);

        auto const cr = ct.find({operation::MUL, f, g});
        if (cr != ct.end())
        {
            return apply(w, cr->second.first.lock());
        }

        auto const x = top_var(f, g);
        auto const r =
            make_branch(x,
                        mul(cof(f, x, true), cof(g, x, true)),
                        mul(cof(f, x, false), cof(g, x, false)));

        ct.insert_or_assign({operation::MUL, f, g}, std::make_pair(r, 0.0));

        return apply(w, r);
    }

    [[nodiscard]] auto regw() const noexcept -> std::int32_t override
    {
        return 0;
    }

    [[deprecated]] auto normw(edge_ptr const& f, edge_ptr const& g) noexcept -> std::int32_t
    {
        assert(f);
        assert(g);
        return std::min(f->w, g->w);
    }

    [[deprecated]] auto static transform(std::vector<phdd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }
};

auto inline phdd::operator+=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->add(f, rhs.f);
    return *this;
}

auto inline phdd::operator-=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);
    return *this;
}

auto inline phdd::operator*=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);
    return *this;
}

auto inline phdd::operator-() const
{
    assert(mgr);

    return phdd{mgr->neg(f), mgr};
}

auto inline phdd::operator~() const
{
    assert(mgr);

    return phdd{mgr->complement(f), mgr};
}

auto inline phdd::operator&=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline phdd::operator|=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline phdd::operator^=(phdd const& rhs) -> phdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline phdd::is_zero() const noexcept
{
    assert(mgr);

    return (*this == mgr->zero());
}

auto inline phdd::is_one() const noexcept
{
    assert(mgr);

    return (*this == mgr->one());
}

auto inline phdd::is_two() const noexcept
{
    assert(mgr);

    return (*this == mgr->two());
}

auto inline phdd::high(bool const weighting) const
{
    assert(mgr);

    return phdd{mgr->high(f, weighting), mgr};
}

auto inline phdd::low(bool const weighting) const
{
    assert(mgr);

    return phdd{mgr->low(f, weighting), mgr};
}

template <typename T, typename... Ts>
auto inline phdd::cof(T const a, Ts... args) const
{
    assert(mgr);

    return phdd{mgr->subfunc(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline phdd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline phdd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline phdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline phdd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline phdd::has_const(std::int32_t const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline phdd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline phdd::ite(phdd const& g, phdd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return phdd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline phdd::compose(std::int32_t const x, phdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return phdd{mgr->compose(f, x, g.f), mgr};
}

auto inline phdd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return phdd{mgr->restr(f, x, a), mgr};
}

auto inline phdd::exist(std::int32_t const x) const
{
    assert(mgr);

    return phdd{mgr->exist(f, x), mgr};
}

auto inline phdd::forall(std::int32_t const x) const
{
    assert(mgr);

    return phdd{mgr->forall(f, x), mgr};
}

auto inline phdd::print() const
{
    assert(mgr);

    mgr->print({*this});
}

}  // namespace freddy::dd