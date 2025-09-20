#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/operation/mul.hpp"   // op::mul
#include "freddy/detail/operation/plus.hpp"  // op::plus

#include <boost/algorithm/string.hpp>  // boost::replace_all

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
class add_manager;

template <typename V>
class add  // multi-terminal binary decision diagram
{
  public:
    add() = default;  // so that MTBDDs initially work with standard containers

    auto operator+=(add const&) -> add&;

    auto operator-=(add const&) -> add&;

    auto operator*=(add const&) -> add&;

    auto operator-() const;

    auto operator~() const;

    auto operator&=(add const&) -> add&;

    auto operator|=(add const&) -> add&;

    auto operator^=(add const&) -> add&;

    auto friend operator+(add lhs, add const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    auto friend operator-(add lhs, add const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    auto friend operator*(add lhs, add const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    auto friend operator&(add lhs, add const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(add lhs, add const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(add lhs, add const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(add const& lhs, add const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same MTBDD manager

        return lhs.f == rhs.f;
    }

    auto friend operator!=(add const& lhs, add const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, add const& g) -> std::ostream&
    {
        s << "Wrapper: " << g.f;
        s << "\nMTBDD manager: " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(add const& g) const noexcept
    {
        assert(f);

        return f->ch() == g.f->ch();
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
    [[nodiscard]] auto fn(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto has_const(V) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(add const&, add const&) const;

    [[nodiscard]] auto compose(std::int32_t, add const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print(std::ostream& = std::cout) const;

  private:
    friend add_manager<V>;

    // wrapper is controlled by its MTBDD manager
    add(std::shared_ptr<detail::edge<bool, V>> f, add_manager<V>* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, V>> f;

    add_manager<V>* mgr{};
};

template <typename V>  // codomain is an arbitrary finite set
    requires std::floating_point<V> || std::integral<V>
class add_manager : public detail::manager<bool, V>
{
  public:
    friend add<V>;

    add_manager() :
            detail::manager<bool, V>{tmls()}
    {
        this->make_const(false, 2, true);
        this->make_const(false, -1, true);
    }

    auto var(std::string_view l = {})
    {
        return add{this->make_var(expansion::S, l), this};
    }

    /*auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < this->var_count());

        return add{this->get_var(i), this};
    }*/

    auto constant(V const val)
    {
        return add{this->make_const(false, val), this};
    }

    auto zero() noexcept
    {
        return add{this->get_const(0), this};
    }

    auto one() noexcept
    {
        return add{this->get_const(1), this};
    }

    auto two() noexcept
    {
        return add{this->get_const(2), this};
    }

    [[nodiscard]] auto size(std::vector<add<V>> const& fs) const
    {
        return this->node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<add<V>> const& fs) const
    {
        assert(!fs.empty());

        return this->longest_path(transform(fs));
    }

    auto print(std::vector<add<V>> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        std::ostringstream buf;
        this->to_dot(transform(fs), outputs, buf);

        auto dot = buf.str();
        boost::replace_all(dot, "label=\" 0 \"]", "]");
        s << dot;
    }

  private:
    using edge_ptr = std::shared_ptr<detail::edge<bool, V>>;

    static auto tmls() -> std::array<edge_ptr, 2>
    {
        auto const leaf0 = std::make_shared<detail::node<bool, V>>(static_cast<V>(0));
        auto const leaf1 = std::make_shared<detail::node<bool, V>>(static_cast<V>(1));

        return std::array<edge_ptr, 2>{std::make_shared<detail::edge<bool, V>>(false, leaf0),
                                       std::make_shared<detail::edge<bool, V>>(false, leaf1)};
    }

    static auto transform(std::vector<add<V>> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return mul(this->get_const(3), f);
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

        return sub(plus(f, g), mul(this->get_const(2), mul(f, g)));
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == this->get_const(0))
        {  // 0 + g = g
            return g;
        }
        if (g == this->get_const(0))
        {  // f + 0 = f
            return f;
        }

        if (f->ch()->is_const() && g->ch()->is_const())
        {
            return this->make_const(false, f->ch()->value() + g->ch()->value());
        }

        op::plus op{f, g};
        if (auto const* const ent = this->cached(op))
        {
            return ent->result();
        }

        auto const x = this->top_var(f, g);

        op.result() = make_branch(x, plus(this->cof(f, x, true), this->cof(g, x, true)),
                                  plus(this->cof(f, x, false), this->cof(g, x, false)));
        return this->cache(std::move(op))->result();
    }

    [[nodiscard]] auto agg([[maybe_unused]] bool const& w, V const& val) const noexcept -> V override
    {
        return val;
    }

    [[nodiscard]] auto comb([[maybe_unused]] bool const& w1, [[maybe_unused]] bool const& w2) const noexcept
        -> bool override
    {
        return false;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return sub(this->get_const(1), f);
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

        return sub(plus(f, g), mul(f, g));
    }

    auto make_branch(detail::var_index const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
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

    auto merge([[maybe_unused]] V const& val1, [[maybe_unused]] V const& val2) const noexcept -> V override
    {
        return 0;  // as no Davio expansion is used
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == this->get_const(0) || g == this->get_const(0))
        {  // f/g * 0 = 0
            return this->get_const(0);
        }
        if (f == this->get_const(1))
        {  // 1 * g = g
            return g;
        }
        if (g == this->get_const(1))
        {  // f * 1 = f
            return f;
        }

        if (f->ch()->is_const() && g->ch()->is_const())
        {
            return this->make_const(false, f->ch()->value() * g->ch()->value());
        }

        op::mul op{f, g};
        if (auto const* const ent = this->cached(op))
        {
            return ent->result();
        }

        auto const x = this->top_var(f, g);

        op.result() = make_branch(x, mul(this->cof(f, x, true), this->cof(g, x, true)),
                                  mul(this->cof(f, x, false), this->cof(g, x, false)));
        return this->cache(std::move(op))->result();
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;
    }
};

template <typename V>
inline auto add<V>::operator+=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->plus(f, rhs.f);
    return *this;
}

template <typename V>
inline auto add<V>::operator-=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);
    return *this;
}

template <typename V>
inline auto add<V>::operator*=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);
    return *this;
}

template <typename V>
inline auto add<V>::operator-() const
{
    assert(mgr);

    return add{mgr->neg(f), mgr};
}

template <typename V>
inline auto add<V>::operator~() const
{
    assert(mgr);

    return add{mgr->complement(f), mgr};
}

template <typename V>
inline auto add<V>::operator&=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

template <typename V>
inline auto add<V>::operator|=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

template <typename V>
inline auto add<V>::operator^=(add const& rhs) -> add&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

template <typename V>
inline auto add<V>::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

template <typename V>
inline auto add<V>::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

template <typename V>
inline auto add<V>::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

template <typename V>
inline auto add<V>::high() const
{
    assert(mgr);
    assert(!f->ch()->is_const());

    return add{f->ch()->br().hi, mgr};
}

template <typename V>
inline auto add<V>::low() const
{
    assert(mgr);
    assert(!f->ch()->is_const());

    return add{f->ch()->br().lo, mgr};
}

template <typename V>
template <typename T, typename... Ts>
inline auto add<V>::fn(T const a, Ts... args) const
{
    assert(mgr);

    return add{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

template <typename V>
inline auto add<V>::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

template <typename V>
inline auto add<V>::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

template <typename V>
inline auto add<V>::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

template <typename V>
inline auto add<V>::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(std::cmp_equal(as.size(), mgr->var_count()));

    return mgr->eval(f, as);
}

template <typename V>
inline auto add<V>::has_const(V const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

template <typename V>
inline auto add<V>::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

template <typename V>
inline auto add<V>::ite(add const& g, add const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return add{mgr->ite(f, g.f, h.f), mgr};
}

template <typename V>
inline auto add<V>::compose(std::int32_t const x, add const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return add{mgr->compose(f, x, g.f), mgr};
}

template <typename V>
inline auto add<V>::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return add{mgr->restr(f, x, a), mgr};
}

template <typename V>
inline auto add<V>::exist(std::int32_t const x) const
{
    assert(mgr);

    return add{mgr->exist(f, x), mgr};
}

template <typename V>
inline auto add<V>::forall(std::int32_t const x) const
{
    assert(mgr);

    return add{mgr->forall(f, x), mgr};
}

template <typename V>
inline auto add<V>::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
