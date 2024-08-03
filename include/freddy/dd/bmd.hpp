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

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

class bmd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class bmd
{
  public:
    bmd() = default;  // so that BMDs initially work with standard containers

    auto operator+=(bmd const&) -> bmd&;

    auto operator-=(bmd const&) -> bmd&;

    auto operator*=(bmd const&) -> bmd&;

    auto operator-() const;

    auto operator~() const;

    auto operator&=(bmd const&) -> bmd&;

    auto operator|=(bmd const&) -> bmd&;

    auto operator^=(bmd const&) -> bmd&;

    auto friend operator+(bmd lhs, bmd const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    auto friend operator-(bmd lhs, bmd const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    auto friend operator*(bmd lhs, bmd const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    auto friend operator&(bmd lhs, bmd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(bmd lhs, bmd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(bmd lhs, bmd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(bmd const& lhs, bmd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BMD manager

        return (lhs.f == rhs.f);
    }

    auto friend operator!=(bmd const& lhs, bmd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bmd const& g) -> std::ostream&
    {
        s << "Wrapper = " << g.f;
        s << "\nBMD manager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(bmd const& g) const noexcept
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

    [[nodiscard]] auto ite(bmd const&, bmd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bmd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print() const;

  private:
    friend bmd_manager;

    // wrapper is controlled by its BMD manager
    bmd(std::shared_ptr<detail::edge<std::int32_t, std::int32_t>> f, bmd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<std::int32_t, std::int32_t>> f;

    bmd_manager* mgr{};
};

class bmd_manager : public detail::manager<std::int32_t, std::int32_t>
{
  public:
    friend bmd;

    bmd_manager() :
            manager(tmls())
    {
        consts.push_back(make_const(2, 1));
        consts.push_back(make_const(-1, 1));
    }

    auto var(std::string_view l = {})
    {
        return bmd{make_var(expansion::PD, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bmd{vars[i], this};
    }

    auto constant(std::int32_t const w)
    {
        return bmd{make_const(w, 1), this};
    }

    auto zero() noexcept
    {
        return bmd{consts[0], this};
    }

    auto one() noexcept
    {
        return bmd{consts[1], this};
    }

    auto two() noexcept
    {
        return bmd{consts[2], this};
    }

    [[nodiscard]] auto size(std::vector<bmd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bmd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto weighted_sum(std::vector<bmd> const& fs)
    {
        auto r = consts[0];
        for (auto i = 0; i < static_cast<std::int32_t>(fs.size()); ++i)
        {  // LSB ... MSB
            r = add(r, mul(make_const(static_cast<std::int32_t>(std::pow(2, i)), 1), fs[i].f));
        }
        return bmd{r, this};
    }

    auto twos_complement(std::vector<bmd> const& fs)
    {
        assert(!fs.empty());

        return bmd{add(apply(static_cast<std::int32_t>(-std::pow(2, fs.size() - 1)), fs.back().f),
                       weighted_sum({fs.begin(), fs.end() - 1}).f),
                   this};
    }

    auto print(std::vector<bmd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        to_dot(transform(fs), outputs, s);
    }

  private:
    using int_edge = detail::edge<std::int32_t, std::int32_t>;

    using int_node = detail::node<std::int32_t, std::int32_t>;

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return ((f == consts[0]) ? f : mul(consts[3], f));
    }

    auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return add(f, neg(g));
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return sub(add(f, g), mul(consts[2], mul(f, g)));
    }

    auto rearrange(operation const op, edge_ptr& f, edge_ptr& g)
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

                assert(w != 0);

                f = foa(std::make_shared<int_edge>(f->w / w, f->v));
                g = foa(std::make_shared<int_edge>(g->w / w, g->v));
                break;
            case operation::MUL:
                w = f->w * g->w;

                if (f->v->operator()() <= g->v->operator()())
                {
                    std::swap(f, g);
                }

                f = foa(std::make_shared<int_edge>(1, f->v));
                g = foa(std::make_shared<int_edge>(1, g->v));
                break;
            default: assert(false);
        }
        return w;
    }

    auto apply(std::int32_t const& w, edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        if (w == 1)
        {
            return f;
        }
        if (w == 0 || f->w == 0)
        {
            return consts[0];
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
        if (f->v == g->v)
        {
            return ((f->w + g->w == 0) ? consts[0] : foa(std::make_shared<int_edge>(f->w + g->w, f->v)));
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

    [[nodiscard]] auto agg(std::int32_t const& w, std::int32_t const& val) const noexcept -> std::int32_t override
    {
        return (w * val);
    }

    [[nodiscard]] auto comb(std::int32_t const& w1, std::int32_t const& w2) const noexcept -> std::int32_t override
    {
        return (w1 * w2);
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return sub(consts[1], f);
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return mul(f, g);
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
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

    auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == consts[0])  // redundancy rule
        {
            return lo;
        }

        auto const w = normw(hi, lo);

        assert(w != 0);

        return ((w != 1) ? foa(std::make_shared<int_edge>(
                               w, foa(std::make_shared<int_node>(x, foa(std::make_shared<int_edge>(hi->w / w, hi->v)),
                                                                 foa(std::make_shared<int_edge>(lo->w / w, lo->v))))))
                         : foa(std::make_shared<int_edge>(w, foa(std::make_shared<int_node>(x, hi, lo)))));
    }

    [[nodiscard]] auto merge(std::int32_t const& val1, std::int32_t const& val2) const noexcept -> std::int32_t override
    {
        return (val1 + val2);
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == consts[0] || g == consts[0])
        {
            return consts[0];
        }
        if (f->v->is_const())
        {
            return apply(f->w, g);
        }
        if (g->v->is_const())
        {
            return apply(g->w, f);
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
                        add(mul(cof(f, x, true), cof(g, x, true)),
                            add(mul(cof(f, x, true), cof(g, x, false)), mul(cof(f, x, false), cof(g, x, true)))),
                        mul(cof(f, x, false), cof(g, x, false)));

        ct.insert_or_assign({operation::MUL, f, g}, std::make_pair(r, 0.0));

        return apply(w, r);
    }

    [[nodiscard]] auto regw() const noexcept -> std::int32_t override
    {
        return 1;
    }

    auto static normw(edge_ptr const& f, edge_ptr const& g) noexcept -> std::int32_t
    {
        assert(f);
        assert(g);

        return ((g->w < 0 || (f->w < 0 && g->w == 0)) ? -std::gcd(f->w, g->w) : std::gcd(f->w, g->w));
    }

    auto static transform(std::vector<bmd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        auto const leaf = std::make_shared<int_node>(1);

        return std::array<edge_ptr, 2>{std::make_shared<int_edge>(0, leaf), std::make_shared<int_edge>(1, leaf)};
    }
};

auto inline bmd::operator+=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->add(f, rhs.f);
    return *this;
}

auto inline bmd::operator-=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);
    return *this;
}

auto inline bmd::operator*=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);
    return *this;
}

auto inline bmd::operator-() const
{
    assert(mgr);

    return bmd{mgr->neg(f), mgr};
}

auto inline bmd::operator~() const
{
    assert(mgr);

    return bmd{mgr->complement(f), mgr};
}

auto inline bmd::operator&=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline bmd::operator|=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline bmd::operator^=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline bmd::is_zero() const noexcept
{
    assert(mgr);

    return (*this == mgr->zero());
}

auto inline bmd::is_one() const noexcept
{
    assert(mgr);

    return (*this == mgr->one());
}

auto inline bmd::is_two() const noexcept
{
    assert(mgr);

    return (*this == mgr->two());
}

auto inline bmd::high(bool const weighting) const
{
    assert(mgr);

    return bmd{mgr->high(f, weighting), mgr};
}

auto inline bmd::low(bool const weighting) const
{
    assert(mgr);

    return bmd{mgr->low(f, weighting), mgr};
}

template <typename T, typename... Ts>
auto inline bmd::cof(T const a, Ts... args) const
{
    assert(mgr);

    return bmd{mgr->subfunc(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline bmd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bmd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bmd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bmd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline bmd::has_const(std::int32_t const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline bmd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline bmd::ite(bmd const& g, bmd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return bmd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline bmd::compose(std::int32_t const x, bmd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bmd{mgr->compose(f, x, g.f), mgr};
}

auto inline bmd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bmd{mgr->restr(f, x, a), mgr};
}

auto inline bmd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bmd{mgr->exist(f, x), mgr};
}

auto inline bmd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bmd{mgr->forall(f, x), mgr};
}

auto inline bmd::print() const
{
    assert(mgr);

    mgr->print({*this});
}

}  // namespace freddy::dd
