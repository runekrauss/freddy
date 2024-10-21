#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/add.hpp"          // op::add
#include "freddy/op/mul.hpp"          // op::mul

#include <algorithm>    // std::transform
#include <array>        // std::array
#include <bit>          // std::bit_width
#include <cassert>      // assert
#include <cmath>        // std::pow, std::signbit
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

// =====================================================================================================================
// Types
// =====================================================================================================================

using edge_weight = std::pair<bool,int32_t>;
using phdd_edge = freddy::detail::edge<edge_weight, float>;
using phdd_node = freddy::detail::node<edge_weight, float>;

// =====================================================================================================================
// std namespace
// =====================================================================================================================

// implements hashing and string view for std::pair<bool, int32_t>
namespace std
{
    template <>
    struct [[maybe_unused]] hash<edge_weight> {
        auto operator()(const edge_weight &v) const -> std::size_t
        {
            return std::hash<int>()(v.second) ^ (v.first << 31);
        }
    };

    std::ostream & operator << (std::ostream & os, const edge_weight & val)
    {
        os << (val.first ? "neg\n" : "") << val.second;
        return os;
    }
}  // namespace std

// =====================================================================================================================
// freddy::dd namespace
// =====================================================================================================================

namespace freddy::dd
{
class phdd_manager;
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

    [[nodiscard]] auto ite(phdd const&, phdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, phdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print(std::ostream& =std::cout) const;

  private:
    friend phdd_manager;

    // wrapper is controlled by its phdd manager
    phdd(std::shared_ptr<phdd_edge> f, phdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<phdd_edge> f;

    phdd_manager* mgr{};
};

class phdd_manager : public detail::manager<edge_weight, float>
{
  private:
    auto static factorize_pow2(int w) -> std::pair<int,int>
    {
        const unsigned int pow = w & (-w);
        auto exp = std::bit_width(pow) - 1;
        return {exp, w / pow};
    }

    auto static decompose_float(float x) -> std::tuple<bool, int32_t ,int32_t>
    {
        assert(!isnanf(x) && !isinff(x) && std::isnormal(x));
        if(x == 0)
        {
            return {0,0,0};
        }
        bool const sign = std::signbit(x);
        int32_t const bits = *reinterpret_cast<int32_t*>(&x);
        int32_t const exponent = ((bits >> 23) & 0xFF) - 127 - 23; // bias 127, sig_size 23
        int32_t const significant = (bits & 0x7FFFFF) | (1 << 23); // leading zero
        auto factors = factorize_pow2(significant);

        return {sign, exponent + factors.first, factors.second};
    }

  public:
    friend phdd;

    phdd_manager() :
            manager(tmls())
    {
        consts.push_back(make_const({false,1}, 1)); // two
        consts.push_back(make_const({true, 0}, 1)); // negative one
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        auto const zero_leaf = std::make_shared<phdd_node>(0.0);
        auto const one_leaf = std::make_shared<phdd_node>(1.0);
        return std::array<edge_ptr, 2>{std::make_shared<phdd_edge>(std::make_pair(false, 0), zero_leaf), // zero
                                       std::make_shared<phdd_edge>(std::make_pair(false, 0), one_leaf)}; // one
    }

    auto var(expansion const d, std::string_view l = {})
    {
        return phdd{make_var(d, l), this};
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

    auto constant(float const w)
    {
        if(w == 0)
        {
            return zero();
        }
        auto d = decompose_float(w);
        return phdd{make_const({std::get<0>(d),std::get<1>(d)}, static_cast<float>(std::get<2>(d))), this};
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
            r = add(r, mul(make_const({0, i}, 1), fs[i].f));
        }
        return phdd{r, this};
    }

    auto print(std::vector<phdd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        to_dot(transform(fs), outputs, s);
    }

  private:

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

    auto apply(edge_weight const& w, edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        if (f == consts[0])
        {
            return consts[0];
        }
        if (!w.first and w.second == 0)
        {
            return f;
        }

        return uedge(comb(w, f->w), f->v);
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
            auto val = agg(f->w,f->v->c()) + agg(g->w,g->v->c());
            return this->constant(val).f;
        }

        if (f->v == g->v and
            f->w.first != g->w.first and
            f->w.second == g->w.second)
        {
            return consts[0];
        }

        if (std::abs(f->w.second) <= std::abs(g->w.second))
        {
            std::swap(f, g);
        }
        auto const w = normw(f, g);
        f = uedge({f->w.first ^ w.first, f->w.second - w.second}, f->v);
        g = uedge({g->w.first ^ w.first, g->w.second - w.second}, g->v);

        op::add op{f, g};
        if (auto const* const ent = cached(op))
        {
            return apply(w, ent->r);
        }

        auto const x = top_var(f, g);
        auto const r = make_branch(x, add(cof(f, x, true), cof(g, x, true)),
                                      add(cof(f, x, false), cof(g, x, false)));

        op.r = r;
        cache(std::move(op));

        return apply(w, r);
    }

    [[nodiscard]] auto agg(edge_weight const& w, float const& val) const noexcept -> float override
    {
        return w.first ? -1 * pow(2, w.second) * val : pow(2, w.second) * val;
    }

    [[nodiscard]] auto comb(edge_weight const& w1, edge_weight const& w2) const noexcept -> edge_weight override
    {
        return {w1.first ^ w2.first, w1.second + w2.second};
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

    [[nodiscard]] auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if(vl[x].t == expansion::S and hi == lo)
        {
            return hi;
        }
        if(vl[x].t == expansion::S and hi == consts[0])
        {
            return uedge(lo->w, unode(x, hi, uedge({false, 0}, lo->v)));
        }
        if(vl[x].t == expansion::PD and hi == consts[0])
        {
            return lo;
        }
        if(lo == consts[0])
        {
            return uedge(hi->w, unode(x, uedge({false, 0}, hi->v), lo));
        }

        auto const w = normw(hi, lo);

        return uedge(w, unode(x, uedge({hi->w.first ^ w.first, hi->w.second - w.second}, hi->v),
                                 uedge({lo->w.first ^ w.first, lo->w.second - w.second}, lo->v)));
    }

    [[nodiscard]] auto merge(float const& val1, float const& val2) const noexcept -> float override
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
            return make_const({f->w.first ^ g->w.first,
                               f->w.second + g->w.second},
                               f->v->c() * g->v->c());
        }

        edge_weight const w = {f->w.first ^ g->w.first, f->w.second + g->w.second};
        if (f->v->operator()() <= g->v->operator()())
        {
            std::swap(f, g);
        }
        f = uedge({0,0}, f->v);
        g = uedge({0,0}, g->v);

        op::mul op{f, g};
        if (auto const* const ent = cached(op))
        {
            return apply(w, ent->r);
        }

        auto const x = top_var(f, g);

        edge_ptr r;
        if(vl[x].t == expansion::S)
        {
            r = make_branch(x,
                            mul(cof(f, x, true), cof(g, x, true)),
                            mul(cof(f, x, false), cof(g, x, false)));
        }
        else if(vl[x].t == expansion::PD)
        {
            r = make_branch(x, add(add(mul(cof(f, x, true),  cof(g, x, true)),
                                       mul(cof(f, x, true),  cof(g, x, false))),
                                       mul(cof(f, x, false), cof(g, x, true))),
                               mul(cof(f, x, false), cof(g, x, false)));
        }
        else
        {
            assert(false);
        }

        op.r = r;
        cache(std::move(op));

        return apply(w, r);
    }

    [[nodiscard]] auto regw() const noexcept -> edge_weight override
    {
        return {0,0};
    }

    [[nodiscard]] auto normw(edge_ptr const& f, edge_ptr const& g) noexcept -> edge_weight
    {
        assert(f);
        assert(g);
        if(f == consts[0])
        {
            return g->w;
        }
        if(g == consts[0])
        {
            return f->w;
        }
        return {f->w.first,
                std::min(f->w.second, g->w.second)};
    }

    [[nodiscard]] auto static transform(std::vector<phdd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs(fs.size());
        std::transform(fs.begin(), fs.end(), gs.begin(), [](auto const& g) { return g.f; });
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

auto inline phdd::high() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return phdd{f->v->br().hi, mgr};
}

auto inline phdd::low() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return phdd{f->v->br().lo, mgr};
}

template <typename T, typename... Ts>
auto inline phdd::fn(T const a, Ts... args) const
{
    assert(mgr);

    return phdd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
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

auto inline phdd::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd