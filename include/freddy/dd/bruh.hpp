#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager
#include "freddy/op/antiv.hpp"        // op::antiv
#include "freddy/op/conj.hpp"         // op::conj
#include "freddy/op/ite.hpp"          // op::ite
#include "freddy/op/sharpsat.hpp"     // op::sharpsat

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

class bruh_manager;

class bruh  // binary decision diagram
{
  public:
    bruh() = default;  // so that BRUHs initially work with standard containers

    auto operator~() const;

    auto operator&=(bruh const&) -> bruh&;

    auto operator|=(bruh const&) -> bruh&;

    auto operator^=(bruh const&) -> bruh&;

    auto friend operator&(bruh lhs, bruh const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(bruh lhs, bruh const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(bruh lhs, bruh const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(bruh const& lhs, bruh const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BRUH manager

        return lhs.f == rhs.f;
    }

    auto friend operator!=(bruh const& lhs, bruh const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bruh const& g) -> std::ostream&
    {
        s << "Wrapper: " << g.f;
        s << "\nBRUH manager: " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(bruh const& g) const noexcept
    {
        assert(f);

        return f->v == g.f->v;
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

    [[nodiscard]] auto compose(std::int32_t, bruh const&) const;

    auto print(std::ostream& = std::cout) const;

  private:
    friend bruh_manager;

    // wrapper is controlled by its BRUH manager
    bruh(std::shared_ptr<detail::edge<bool, std::int32_t>> f, bruh_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, std::int32_t>> f;  // DD handle

    bruh_manager* mgr{};  // must be destroyed after this wrapper
};

class bruh_manager : public detail::manager<bool, std::int32_t>
{
  public:
    friend bruh;

    bruh_manager() :
            manager{tmls()}
    {}

    auto var(std::string_view l = {})
    {
        return bruh{make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bruh{vars[i], this};
    }

    auto zero() noexcept
    {
        return bruh{consts[0], this};
    }

    auto one() noexcept
    {
        return bruh{consts[1], this};
    }

    auto new_const(std::int32_t val){
        return bruh{make_const(false, val), this};
    }

    [[nodiscard]] auto size(std::vector<bruh> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bruh> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto print(std::vector<bruh> const& fs, std::vector<std::string> const& outputs = {},
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

    auto static transform(std::vector<bruh> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::ranges::transform(fs, std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return disj(f, g);
    }

    [[nodiscard]] auto agg(bool const& w, std::int32_t const& val) const noexcept -> std::int32_t override
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
        if (f->v->is_const())
        {
            return new_const(1 - f->v->c()).f;
        }

        //TODO: CACHE

        auto const x = f->v->br().x;
        return make_branch(x, complement(cof(f, x, true)), complement(cof(f, x, false)));
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
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
            return new_const(f->v->c() * g->v->c()).f;
        }

        op::conj op{f, g};
        if (auto const* const ent = cached(op))
        {
            return ent->r;
        }

        auto const x = top_var(f, g);

        op.r = make_branch(x, conj(cof(f, x, true), cof(g, x, true)), conj(cof(f, x, false), cof(g, x, false)));
        return cache(std::move(op))->r;
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
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
            return new_const(f->v->c() + g->v->c()).f;
        }

        //TODO: CACHE

        auto const x = top_var(f, g);
        return make_branch(x, disj(cof(f, x, true), cof(g, x, true)), disj(cof(f, x, false), cof(g, x, false)));
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

        return conj(f, g);
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;  // means a regular (non-complemented) edge
    }
};

auto inline bruh::operator~() const
{
    assert(mgr);

    return bruh{mgr->complement(f), mgr};
}

auto inline bruh::operator&=(bruh const& rhs) -> bruh&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline bruh::operator|=(bruh const& rhs) -> bruh&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline bruh::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

auto inline bruh::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

auto inline bruh::high() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return bruh{f->v->br().hi, mgr};
}

auto inline bruh::low() const
{
    assert(mgr);
    assert(!f->v->is_const());

    return bruh{f->v->br().lo, mgr};
}

template <typename T, typename... Ts>
auto inline bruh::fn(T const a, Ts... args) const
{
    assert(mgr);

    return bruh{mgr->fn(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline bruh::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bruh::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bruh::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bruh::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline bruh::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline bruh::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline bruh::compose(std::int32_t const x, bruh const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bruh{mgr->compose(f, x, g.f), mgr};
}

auto inline bruh::print(std::ostream& s) const
{
    assert(mgr);

    mgr->print({*this}, {}, s);
}

}  // namespace freddy::dd
