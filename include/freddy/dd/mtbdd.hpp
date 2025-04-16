#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/add.hpp"          // op::add
#include "freddy/op/mul.hpp"          // op::mul

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <concepts>     // std::floating_point
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
#include <ostream>      // std::ostream
#include <sstream>      // std::ostringstream
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

template <typename V>
    requires std::floating_point<V> || std::integral<V>
class mtbdd_manager;

template <typename V>
class mtbdd  // multi-terminal binary decision diagram
{
  public:
    mtbdd() = default;  // so that MTBDDs initially work with standard containers

    auto operator+=(mtbdd const&) -> mtbdd&;

    auto operator-=(mtbdd const&) -> mtbdd&;

    auto operator*=(mtbdd const&) -> mtbdd&;

    auto operator-() const;

    auto operator~() const;

    auto operator&=(mtbdd const&) -> mtbdd&;

    auto operator|=(mtbdd const&) -> mtbdd&;

    auto operator^=(mtbdd const&) -> mtbdd&;

    auto friend operator+(mtbdd lhs, mtbdd const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    auto friend operator-(mtbdd lhs, mtbdd const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    auto friend operator*(mtbdd lhs, mtbdd const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    auto friend operator&(mtbdd lhs, mtbdd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(mtbdd lhs, mtbdd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(mtbdd lhs, mtbdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(mtbdd const& lhs, mtbdd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same MTBDD manager

        return lhs.f == rhs.f;
    }

    auto friend operator!=(mtbdd const& lhs, mtbdd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, mtbdd const& g) -> std::ostream&
    {
        s << "Wrapper: " << g.f;
        s << "\nMTBDD manager: " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(mtbdd const& g) const noexcept
    {
        assert(f);

        return f->v == g.f->v;
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
    [[nodiscard]] auto fn(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto has_const(V) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(mtbdd const&, mtbdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, mtbdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print(std::ostream& = std::cout) const;

  private:
    friend mtbdd_manager<V>;

    // wrapper is controlled by its MTBDD manager
    mtbdd(std::shared_ptr<detail::edge<bool, V>> f, mtbdd_manager<V>* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, V>> f;

    mtbdd_manager<V>* mgr{};
};

template <typename V>  // codomain is an arbitrary finite set
    requires std::floating_point<V> || std::integral<V>
class mtbdd_manager : public detail::manager<bool, V>
{
  public:
    friend mtbdd<V>;

    mtbdd_manager() :
            detail::manager<bool, V>{tmls()}
    {
        this->consts.push_back(this->make_const(false, 2));
        this->consts.push_back(this->make_const(false, -1));
    }

    auto var(std::string_view l = {})
    {
        return mtbdd{this->make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < this->var_count());

        return mtbdd{this->vars[i], this};
    }

    auto constant(V const val)
    {
        return mtbdd{this->make_const(false, val), this};
    }

    auto zero() noexcept
    {
        return mtbdd{this->consts[0], this};
    }

    auto one() noexcept
    {
        return mtbdd{this->consts[1], this};
    }

    auto two() noexcept
    {
        return mtbdd{this->consts[2], this};
    }

    [[nodiscard]] auto size(std::vector<mtbdd<V>> const& fs) const
    {
        return this->node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<mtbdd<V>> const& fs) const
    {
        assert(!fs.empty());

        return this->longest_path(transform(fs));
    }

    auto print(std::vector<mtbdd<V>> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        std::ostringstream buf;
        this->to_dot(transform(fs), outputs, buf);

        auto dot = buf.str();
        detail::replace_all(dot, "label=\" 0 \"]", "]");
        s << dot;
    }

  private:
    using edge_ptr = std::shared_ptr<detail::edge<bool, V>>;

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        auto const leaf0 = std::make_shared<detail::node<bool, V>>(static_cast<V>(0));
        auto const leaf1 = std::make_shared<detail::node<bool, V>>(static_cast<V>(1));

        return std::array<edge_ptr, 2>{std::make_shared<detail::edge<bool, V>>(false, leaf0),
                                       std::make_shared<detail::edge<bool, V>>(false, leaf1)};
    }

    auto static transform(std::vector<mtbdd<V>> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return mul(this->consts[3], f);
    }

    auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        return add(f, neg(g));
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return sub(add(f, g), mul(this->consts[2], mul(f, g)));
    }

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == this->consts[0])
        {  // 0 + g = g
            return g;
        }
        if (g == this->consts[0])
        {  // f + 0 = f
            return f;
        }

        if (f->v->is_const() && g->v->is_const())
        {
            return this->make_const(false, f->v->c() + g->v->c());
        }

        op::add op{f, g};
        if (auto const* const ent = this->cached(op))
        {
            return ent->r;
        }

        auto const x = this->top_var(f, g);

        op.r = make_branch(x, add(this->cof(f, x, true), this->cof(g, x, true)),
                           add(this->cof(f, x, false), this->cof(g, x, false)));
        return this->cache(std::move(op))->r;
    }

    [[nodiscard]] auto agg([[maybe_unused]] bool const& w, V const& val) const noexcept -> V override
    {
        return val;
    }

    [[nodiscard]] auto comb([[maybe_unused]] bool const& w1,
                            [[maybe_unused]] bool const& w2) const noexcept -> bool override
    {
        return false;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return sub(this->consts[1], f);
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

        return sub(add(f, g), mul(f, g));
    }

    auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < this->var_count());
        assert(hi);
        assert(lo);

        if (hi == lo)
        {
            return hi;
        }
        return this->uedge(false, this->unode(x, std::move(hi), std::move(lo)));
    }

    [[nodiscard]] auto merge([[maybe_unused]] V const& val1,
                             [[maybe_unused]] V const& val2) const noexcept -> V override
    {
        return 0;  // as no Davio expansion is used
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == this->consts[0] || g == this->consts[0])
        {  // f/g * 0 = 0
            return this->consts[0];
        }
        if (f == this->consts[1])
        {  // 1 * g = g
            return g;
        }
        if (g == this->consts[1])
        {  // f * 1 = f
            return f;
        }

        if (f->v->is_const() && g->v->is_const())
        {
            return this->make_const(false, f->v->c() * g->v->c());
        }

        op::mul op{f, g};
        if (auto const* const ent = this->cached(op))
        {
            return ent->r;
        }

        auto const x = this->top_var(f, g);

        op.r = make_branch(x, mul(this->cof(f, x, true), this->cof(g, x, true)),
                           mul(this->cof(f, x, false), this->cof(g, x, false)));
        return this->cache(std::move(op))->r;
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;
    }
};

template <typename V>
auto inline mtbdd<V>::operator+=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->add(f, rhs.f);
    return *this;
}

template <typename V>
auto inline mtbdd<V>::operator-=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);
    return *this;
}

template <typename V>
auto inline mtbdd<V>::operator*=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);
    return *this;
}

template <typename V>
auto inline mtbdd<V>::operator-() const
{
    assert(mgr);

    return mtbdd{mgr->neg(f), mgr};
}

template <typename V>
auto inline mtbdd<V>::operator~() const
{
    assert(mgr);

    return mtbdd{mgr->complement(f), mgr};
}

template <typename V>
auto inline mtbdd<V>::operator&=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

template <typename V>
auto inline mtbdd<V>::operator|=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

template <typename V>
auto inline mtbdd<V>::operator^=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

template <typename V>
auto inline mtbdd<V>::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

template <typename V>
auto inline mtbdd<V>::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

template <typename V>
auto inline mtbdd<V>::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

template <typename V>
auto inline mtbdd<V>::high() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return mtbdd{f->v->br().hi, mgr};
}

template <typename V>
auto inline mtbdd<V>::low() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return mtbdd{f->v->br().lo, mgr};
}

template <typename V>
template <typename T, typename... Ts>
auto inline mtbdd<V>::fn(T const a, Ts... args) const
{
    assert(mgr);

    return mtbdd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

template <typename V>
auto inline mtbdd<V>::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

template <typename V>
auto inline mtbdd<V>::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

template <typename V>
auto inline mtbdd<V>::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

template <typename V>
auto inline mtbdd<V>::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

template <typename V>
auto inline mtbdd<V>::has_const(V const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

template <typename V>
auto inline mtbdd<V>::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

template <typename V>
auto inline mtbdd<V>::ite(mtbdd const& g, mtbdd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return mtbdd{mgr->ite(f, g.f, h.f), mgr};
}

template <typename V>
auto inline mtbdd<V>::compose(std::int32_t const x, mtbdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return mtbdd{mgr->compose(f, x, g.f), mgr};
}

template <typename V>
auto inline mtbdd<V>::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return mtbdd{mgr->restr(f, x, a), mgr};
}

template <typename V>
auto inline mtbdd<V>::exist(std::int32_t const x) const
{
    assert(mgr);

    return mtbdd{mgr->exist(f, x), mgr};
}

template <typename V>
auto inline mtbdd<V>::forall(std::int32_t const x) const
{
    assert(mgr);

    return mtbdd{mgr->forall(f, x), mgr};
}

template <typename V>
auto inline mtbdd<V>::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
