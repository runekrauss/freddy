#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager

#include <algorithm>    // std::transform
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
    bmd() = default;

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
        s << "Wrapper: " << g.f;
        s << "\nManager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto weight() const noexcept
    {
        assert(f);

        return f->w;
    }

    [[nodiscard]] auto equals(bmd const& g) const noexcept
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

    [[nodiscard]] auto is_two() const noexcept;

    [[nodiscard]] auto ite(bmd const&, bmd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bmd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print() const;

  private:
    // wrapper is controlled by its BMD manager
    bmd(std::shared_ptr<detail::edge> f, bmd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge> f;

    bmd_manager* mgr{};

    friend bmd_manager;
};

class bmd_manager : public detail::manager
{
  public:
    auto var(std::string_view l = {})
    {
        return bmd{make_var(detail::decomposition::PD, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bmd{vars[i], this};
    }

    auto constant(std::int32_t const w)
    {
        return bmd{foa(std::make_shared<detail::edge>(w)), this};
    }

    auto zero() noexcept
    {
        return bmd{tmls[0], this};
    }

    auto one() noexcept
    {
        return bmd{tmls[1], this};
    }

    auto two() noexcept
    {
        return bmd{tmls[2], this};
    }

    auto size(std::vector<bmd> const& fs)
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bmd> const& fs) const
    {
        return longest_path(transform(fs));
    }

    auto weighted_sum(std::vector<bmd> const& fs)
    {
        auto r = tmls[0];
        for (auto i = 0; i < static_cast<std::int32_t>(fs.size()); ++i)
        {  // LSB ... MSB
            r = add(r, mul(foa(std::make_shared<detail::edge>(static_cast<std::int32_t>(std::pow(2, i)))), fs[i].f));
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

    auto print(std::vector<bmd> const& fs, std::vector<std::string> outputs = {}, std::ostream& s = std::cout)
    {
        to_dot(transform(fs), std::move(outputs), s);
    }

  private:
    [[nodiscard]] static auto transform(std::vector<bmd> const& fs) -> std::vector<std::shared_ptr<detail::edge>>
    {
        std::vector<std::shared_ptr<detail::edge>> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto sub(std::shared_ptr<detail::edge> const& f, std::shared_ptr<detail::edge> const& g)
    {
        assert(f);
        assert(g);

        return add(f, neg(g));
    }

    auto ite(std::shared_ptr<detail::edge> const& f, std::shared_ptr<detail::edge> const& g,
             std::shared_ptr<detail::edge> const& h)
    {
        assert(f);
        assert(g);
        assert(h);

        return disj(conj(f, g), conj(complement(f), h));
    }

    auto antiv(std::shared_ptr<detail::edge> const& f, std::shared_ptr<detail::edge> const& g)
    {
        assert(f);
        assert(g);

        return sub(add(f, g), mul(tmls[2], mul(f, g)));
    }

    auto rearrange(operation const op, std::shared_ptr<detail::edge>& f, std::shared_ptr<detail::edge>& g)
    {
        assert(f);
        assert(g);

        std::int32_t w{};

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

                f = foa(std::make_shared<detail::edge>(f->w / w, f->v));
                g = foa(std::make_shared<detail::edge>(g->w / w, g->v));
                break;
            case operation::MUL:
                w = f->w * g->w;

                if (f->v->operator()() <= g->v->operator()())
                {
                    std::swap(f, g);
                }

                f = foa(std::make_shared<detail::edge>(1, f->v));
                g = foa(std::make_shared<detail::edge>(1, g->v));
                break;
            default: assert(false);
        }

        return w;
    }

    auto add(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g) -> std::shared_ptr<detail::edge> override
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
        if (f->v == g->v)
        {
            return ((f->w + g->w == 0) ? tmls[0] : foa(std::make_shared<detail::edge>(f->w + g->w, f->v)));
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

    auto apply(std::int32_t const w, std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {
        assert(f);

        if (w == 1)
        {
            return f;
        }
        if (w == 0 || f->w == 0)
        {
            return tmls[0];
        }

        return foa(std::make_shared<detail::edge>(w * f->w, f->v));
    }

    auto complement(std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {
        assert(f);

        return sub(tmls[1], f);
    }

    auto conj(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g)
        -> std::shared_ptr<detail::edge> override
    {
        assert(f);
        assert(g);

        return mul(f, g);
    }

    auto disj(std::shared_ptr<detail::edge> const& f, std::shared_ptr<detail::edge> const& g)
        -> std::shared_ptr<detail::edge> override
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

        return sub(add(f, g), mul(f, g));
    }

    [[nodiscard]] auto is_normalized(std::shared_ptr<detail::edge> const& hi,
                                     std::shared_ptr<detail::edge> const& lo) const noexcept -> bool override
    {
        assert(hi);
        assert(lo);

        return ((hi == tmls[0]) ? false : lo->w < 0 || (hi->w < 0 && lo->w == 0));
    }

    auto make_branch(std::int32_t const x, std::shared_ptr<detail::edge> hi, std::shared_ptr<detail::edge> lo)
        -> std::shared_ptr<detail::edge> override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == tmls[0])  // redundancy rule
        {
            return lo;
        }

        auto const w = normw(hi, lo);

        assert(w != 0);

        return (
            (w != 1)
                ? foa(std::make_shared<detail::edge>(
                      w, foa(std::make_shared<detail::node>(x, foa(std::make_shared<detail::edge>(hi->w / w, hi->v)),
                                                            foa(std::make_shared<detail::edge>(lo->w / w, lo->v))))))
                : foa(std::make_shared<detail::edge>(w, foa(std::make_shared<detail::node>(x, hi, lo)))));
    }

    auto mul(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g) -> std::shared_ptr<detail::edge> override
    {
        assert(f);
        assert(g);

        if (f == tmls[0] || g == tmls[0])
        {
            return tmls[0];
        }
        if (!f->v)
        {
            return apply(f->w, g);
        }
        if (!g->v)
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

    auto neg(std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {
        assert(f);

        return ((f == tmls[0]) ? f : mul(tmls[3], f));
    }

    [[nodiscard]] auto regw() const noexcept -> std::int32_t override
    {
        return 1;
    }

    auto static normw(std::shared_ptr<detail::edge> const& f, std::shared_ptr<detail::edge> const& g) noexcept
        -> std::int32_t
    {
        assert(f);
        assert(g);

        return ((g->w < 0 || (f->w < 0 && g->w == 0)) ? -std::gcd(f->w, g->w) : std::gcd(f->w, g->w));
    }

    friend bmd;
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

template <typename T, typename... Ts>
auto inline bmd::eval(T const a, Ts... args) const
{
    assert(mgr);

    return mgr->eval(f, a, std::forward<Ts>(args)...);
}

auto inline bmd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bmd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bmd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bmd::is_essential(std::int32_t const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
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
