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
#include <cmath>        // std::pow
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
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

class mtbdd_manager;

class mtbdd  // multi-terminal binary decision diagram
{
  public:
    mtbdd() = default;

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
        assert(lhs.mgr == rhs.mgr);

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
    auto fn(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto has_const(bool) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(mtbdd const&, mtbdd const&) const;

    [[nodiscard]] auto compose(std::int32_t, mtbdd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    auto print(std::ostream& = std::cout) const;

  private:
    friend mtbdd_manager;

    mtbdd(std::shared_ptr<detail::edge<bool, std::int32_t>> f, mtbdd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, std::int32_t>> f;  // DD handle

    mtbdd_manager* mgr{};  // must be destroyed after this wrapper
};

class mtbdd_manager : public detail::manager<bool, std::int32_t>
{
  public:
    friend mtbdd;

    mtbdd_manager() :
            manager{tmls()}
    {
        consts.push_back(make_const(false, 2));
        consts.push_back(make_const(false, -1));
    }

    auto var(std::string_view l = {})
    {
        return mtbdd{make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return mtbdd{vars[i], this};
    }

    auto constant(std::int32_t val){
        return mtbdd{make_const(false, val), this};
    }

    auto zero() noexcept
    {
        return mtbdd{consts[0], this};
    }

    auto one() noexcept
    {
        return mtbdd{consts[1], this};
    }

    auto two() noexcept
    {
        return mtbdd{consts[2], this};
    }

    [[nodiscard]] auto size(std::vector<mtbdd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<mtbdd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto print(std::vector<mtbdd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        std::ostringstream buf;  // for highlighting EXP
        to_dot(transform(fs), outputs, buf);

        auto dot = buf.str();
        detail::replace_all(dot, ",label=\" 0 \"]","]");
        s << dot;
    }

  private:
    auto static tmls() -> std::array<edge_ptr, 2>
    {
        // choose the 0-leaf due to complemented edges in order to ensure canonicity
        auto const one = std::make_shared<detail::node<bool, std::int32_t>>(0);
        auto const two = std::make_shared<detail::node<bool, std::int32_t>>(1);

        return std::array<edge_ptr, 2>{std::make_shared<detail::edge<bool, std::int32_t>>(false, one),
                                       std::make_shared<detail::edge<bool, std::int32_t>>(false, two)};
    }

    auto static transform(std::vector<mtbdd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return mul(consts[3], f);
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

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == consts[0])
        { // g + 0 = g
            return g;
        }
        if (g == consts[0])
        { // f + 0 = f
            return f;
        }

        if (f->v->is_const() && g->v->is_const()){
            return constant(f->v->c() + g->v->c()).f;
        }

        op::add op{f, g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = top_var(f, g);

        op.r = make_branch(x, add(cof(f, x, true), cof(g, x, true)), add(cof(f, x, false), cof(g, x, false)));
        return cache(std::move(op))->r;
    }

    [[nodiscard]] auto agg([[maybe_unused]] bool const& w, std::int32_t const& val) const noexcept -> std::int32_t override
    {
        return val;
    }

    [[nodiscard]] auto comb([[maybe_unused]] bool const& w1, [[maybe_unused]] bool const& w2) const noexcept -> bool override
    {
        return false;
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

        return mul(f,g);
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return sub(add(f, g), mul(f, g));
    }

    auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == lo)
        {
            return hi;
        }
        return uedge(false, unode(x, std::move(hi), std::move(lo)));
    }

    [[nodiscard]] auto merge(std::int32_t const& val1, std::int32_t const& val2) const noexcept -> std::int32_t override
    {
        return !(val1 == val2);
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == consts[0] || g == consts[0])
        {  // f/g * 0 = 0
            return consts[0];
        }
        if (f == consts[1])
        {  // g * 1 = g
            return g;
        }
        if (g == consts[1])
        {  // f * 1 = f
            return f;
        }

        if (f->v->is_const() && g->v->is_const()){
            return constant(f->v->c() * g->v->c()).f;
        }

        op::mul op{f, g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = top_var(f, g);

        op.r = make_branch(x, mul(cof(f, x, true), cof(g, x, true)), mul(cof(f, x, false), cof(g, x, false)));
        return cache(std::move(op))->r;
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;  // means a regular edge
    }
};

auto inline mtbdd::operator+=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->add(f, rhs.f);
    return *this;
}

auto inline mtbdd::operator-=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);
    return *this;
}

auto inline mtbdd::operator*=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);
    return *this;
}

auto inline mtbdd::operator-() const
{
    assert(mgr);

    return mtbdd{mgr->neg(f), mgr};
}

auto inline mtbdd::operator~() const
{
    assert(mgr);

    return mtbdd{mgr->complement(f), mgr};
}

auto inline mtbdd::operator&=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline mtbdd::operator|=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline mtbdd::operator^=(mtbdd const& rhs) -> mtbdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline mtbdd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

auto inline mtbdd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

auto inline mtbdd::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

auto inline mtbdd::high() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return mtbdd{f->v->br().hi, mgr};
}

auto inline mtbdd::low() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return mtbdd{f->v->br().lo, mgr};
}

template <typename T, typename... Ts>
auto inline mtbdd::fn(T const a, Ts... args) const
{
    assert(mgr);

    return mtbdd{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline mtbdd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline mtbdd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline mtbdd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline mtbdd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline mtbdd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline mtbdd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline mtbdd::ite(mtbdd const& g, mtbdd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return mtbdd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline mtbdd::compose(std::int32_t const x, mtbdd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return mtbdd{mgr->compose(f, x, g.f), mgr};
}

auto inline mtbdd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return mtbdd{mgr->restr(f, x, a), mgr};
}

auto inline mtbdd::exist(std::int32_t const x) const
{
    assert(mgr);

    return mtbdd{mgr->exist(f, x), mgr};
}

auto inline mtbdd::forall(std::int32_t const x) const
{
    assert(mgr);

    return mtbdd{mgr->forall(f, x), mgr};
}

auto inline mtbdd::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
