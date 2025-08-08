#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/mul.hpp"          // op::mul
#include "freddy/op/plus.hpp"         // op::plus

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::pow
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
#include <numeric>      // std::gcd
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

class bmd_manager;

class bmd  // binary moment diagram
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

        return lhs.f == rhs.f;
    }

    auto friend operator!=(bmd const& lhs, bmd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bmd const& g) -> std::ostream&
    {
        s << "Wrapper: " << g.f;
        s << "\nBMD manager: " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(bmd const& g) const noexcept
    {
        assert(f);

        return f->ch() == g.f->ch();
    }

    [[nodiscard]] auto weight() const noexcept
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

    [[nodiscard]] auto is_two() const noexcept;

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

    [[nodiscard]] auto has_const(std::int32_t) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(bmd const&, bmd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bmd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print(std::ostream& = std::cout) const;

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
            manager{tmls()}
    {
        make_const(2, 1, true);
        make_const(-1, 1, true);
    }

    auto var(std::string_view l = {})
    {
        return bmd{make_var(expansion::PD, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        //assert(i < var_count());

        return bmd{get_var(i), this};
    }

    auto constant(std::int32_t const w)
    {
        return bmd{make_const(w, 1), this};
    }

    auto zero() noexcept
    {
        return bmd{get_const(0), this};
    }

    auto one() noexcept
    {
        return bmd{get_const(1), this};
    }

    auto two() noexcept
    {
        return bmd{get_const(2), this};
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
        auto r = get_const(0);
        for (decltype(fs.size()) i = 0; i < fs.size(); ++i)
        {  // LSB...MSB
            r = plus(r, mul(make_const(static_cast<std::int32_t>(std::pow(2, i)), 1), fs[i].f));
        }
        return bmd{r, this};
    }

    auto twos_complement(std::vector<bmd> const& fs)
    {
        assert(!fs.empty());

        return bmd{plus(apply(static_cast<std::int32_t>(-std::pow(2, fs.size() - 1)), fs.back().f),
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
    static auto normw(edge_ptr const& f, edge_ptr const& g) noexcept
    {
        assert(f);
        assert(g);

        return g->weight() < 0 || (f->weight() < 0 && g->weight() == 0) ? -std::gcd(f->weight(), g->weight())
                                                                        : std::gcd(f->weight(), g->weight());
    }

    static auto tmls() -> std::array<edge_ptr, 2>
    {
        auto const leaf = std::make_shared<detail::node<std::int32_t, std::int32_t>>(1);

        return std::array<edge_ptr, 2>{std::make_shared<detail::edge<std::int32_t, std::int32_t>>(0, leaf),
                                       std::make_shared<detail::edge<std::int32_t, std::int32_t>>(1, leaf)};
    }

    static auto transform(std::vector<bmd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return f == get_const(0) ? f : mul(get_const(3), f);
    }

    auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return plus(f, neg(g));
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return sub(plus(f, g), mul(get_const(2), mul(f, g)));
    }

    auto apply(std::int32_t const& w, edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        if (w == 1)
        {
            return f;
        }
        if (w == 0 || f->weight() == 0)
        {
            return get_const(0);
        }
        return uedge(comb(w, f->weight()), f->ch());
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == get_const(0))
        {
            return g;
        }
        if (g == get_const(0))
        {
            return f;
        }
        if (f->ch() == g->ch())
        {
            return f->weight() + g->weight() == 0 ? get_const(0) : uedge(f->weight() + g->weight(), f->ch());
        }

        // increase the probability of reusing previously computed results (rearrange)
        std::int32_t w = 0;
        if (std::abs(f->weight()) <= std::abs(g->weight()))
        {
            std::swap(f, g);
            w = normw(f, g);
        }
        else
        {
            w = normw(g, f);
        }
        f = uedge(f->weight() / w, f->ch());
        g = uedge(g->weight() / w, g->ch());

        op::plus op{f, g};
        if (auto const* const ent = cached(op))
        {
            return apply(w, ent->result());
        }

        auto const x = top_var(f, g);
        auto const r = make_branch(x, plus(cof(f, x, true), cof(g, x, true)), plus(cof(f, x, false), cof(g, x, false)));

        op.result() = r;
        cache(std::move(op));

        return apply(w, r);
    }

    [[nodiscard]] auto agg(std::int32_t const& w, std::int32_t const& val) const noexcept -> std::int32_t override
    {
        return w * val;
    }

    [[nodiscard]] auto comb(std::int32_t const& w1, std::int32_t const& w2) const noexcept -> std::int32_t override
    {
        return w1 * w2;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return sub(get_const(1), f);
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

        if (f == get_const(0))
        {
            return g;
        }
        if (g == get_const(0))
        {
            return f;
        }
        return sub(plus(f, g), mul(f, g));
    }

    auto make_branch(detail::var_index const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == get_const(0))  // redundancy rule
        {
            return lo;
        }

        auto const w = normw(hi, lo);

        assert(w != 0);

        return w != 1 ? uedge(w, unode(x, uedge(hi->weight() / w, hi->ch()), uedge(lo->weight() / w, lo->ch())))
                      : uedge(w, unode(x, hi, lo));
    }

    auto merge(std::int32_t const& val1, std::int32_t const& val2) const noexcept -> std::int32_t override
    {
        return val1 + val2;
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == get_const(0) || g == get_const(0))
        {
            return get_const(0);
        }
        if (f->ch()->is_const())
        {
            return apply(f->weight(), g);
        }
        if (g->ch()->is_const())
        {
            return apply(g->weight(), f);
        }

        // rearrange
        auto const w = f->weight() * g->weight();
        if (f->ch()->operator()() <= g->ch()->operator()())
        {
            std::swap(f, g);
        }
        f = uedge(1, f->ch());
        g = uedge(1, g->ch());

        op::mul op{f, g};
        if (auto const* const ent = cached(op))
        {
            return apply(w, ent->result());
        }

        auto const x = top_var(f, g);
        auto const r =
            make_branch(x,
                        plus(mul(cof(f, x, true), cof(g, x, true)),
                             plus(mul(cof(f, x, true), cof(g, x, false)), mul(cof(f, x, false), cof(g, x, true)))),
                        mul(cof(f, x, false), cof(g, x, false)));

        op.result() = r;
        cache(std::move(op));

        return apply(w, r);
    }

    [[nodiscard]] auto regw() const noexcept -> std::int32_t override
    {
        return 1;
    }
};

inline auto bmd::operator+=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->plus(f, rhs.f);
    return *this;
}

inline auto bmd::operator-=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);
    return *this;
}

inline auto bmd::operator*=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);
    return *this;
}

inline auto bmd::operator-() const
{
    assert(mgr);

    return bmd{mgr->neg(f), mgr};
}

inline auto bmd::operator~() const
{
    assert(mgr);

    return bmd{mgr->complement(f), mgr};
}

inline auto bmd::operator&=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

inline auto bmd::operator|=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

inline auto bmd::operator^=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

inline auto bmd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

inline auto bmd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

inline auto bmd::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

inline auto bmd::high() const
{
    assert(mgr);
    assert(!f->ch()->is_const());

    return bmd{f->ch()->br().hi, mgr};
}

inline auto bmd::low() const
{
    assert(mgr);
    assert(!f->ch()->is_const());

    return bmd{f->ch()->br().lo, mgr};
}

template <typename T, typename... Ts>
inline auto bmd::fn(T const a, Ts... args) const
{
    assert(mgr);

    return bmd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

inline auto bmd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

inline auto bmd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

inline auto bmd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

inline auto bmd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(std::cmp_equal(as.size(), mgr->var_count()));

    return mgr->eval(f, as);
}

inline auto bmd::has_const(std::int32_t const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

inline auto bmd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

inline auto bmd::ite(bmd const& g, bmd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return bmd{mgr->ite(f, g.f, h.f), mgr};
}

inline auto bmd::compose(std::int32_t const x, bmd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bmd{mgr->compose(f, x, g.f), mgr};
}

inline auto bmd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bmd{mgr->restr(f, x, a), mgr};
}

inline auto bmd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bmd{mgr->exist(f, x), mgr};
}

inline auto bmd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bmd{mgr->forall(f, x), mgr};
}

inline auto bmd::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
