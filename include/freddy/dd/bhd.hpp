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
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::make_pair
#include <vector>       // std::vector

#include <random>

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

class bhd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class bhd
{
  public:
    bhd() = default;  // so that it initially works with standard containers

    auto operator~() const;

    auto operator&=(bhd const&) -> bhd&;

    auto operator|=(bhd const&) -> bhd&;

    auto operator^=(bhd const&) -> bhd&;

    auto friend operator&(bhd lhs, bhd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(bhd lhs, bhd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(bhd lhs, bhd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(bhd const& lhs, bhd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BHD manager

        return (lhs.f == rhs.f);
    }

    auto friend operator!=(bhd const& lhs, bhd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bhd const& g) -> std::ostream&
    {
        s << "Wrapper: " << g.f;
        s << "\nManager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto is_complemented() const noexcept
    {
        assert(f);

        return (f->w != 0 || f->w != -1);
    }

    [[nodiscard]] auto equals(bhd const& g) const noexcept
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

    [[nodiscard]] auto ite(bhd const&, bhd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bhd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    [[nodiscard]] auto satcount() const;

    auto print() const;

    auto test() const;

  private:
    // wrapper is controlled by its BHD manager
    bhd(std::shared_ptr<detail::edge> f, bhd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge> f;  // DD handle

    bhd_manager* mgr{};  // must be destroyed after this wrapper

    friend bhd_manager;
};

class bhd_manager : public detail::manager
{
  public:

    bhd_manager()
    {
        tmls[2] = foa(std::make_shared<freddy::detail::edge>(-10));
        tmls[3] = foa(std::make_shared<freddy::detail::edge>(-11));
    }

    auto getExp(){
        return bhd{tmls[2], this};
    }

    auto var(std::string_view l = {})
    {
        return bhd{make_var(detail::decomposition::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bhd{vars[i], this};
    }

    auto zero() noexcept
    {
        return bhd{tmls[0], this};
    }

    auto one() noexcept
    {
        return bhd{tmls[1], this};
    }

    auto size(std::vector<bhd> const& fs)
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bhd> const& fs) const
    {
        return longest_path(transform(fs));
    }

    auto print(std::vector<bhd> const& fs, std::vector<std::string> outputs = {}, std::ostream& s = std::cout)
    {
        to_dot(transform(fs), std::move(outputs), s);
    }


  private:

    [[nodiscard]] static auto transform(std::vector<bhd> const& fs) -> std::vector<std::shared_ptr<detail::edge>>
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

    void newExtensionNodeFromTest(std::vector<std::pair<std::int32_t, bool>> path){

        for (auto e : path){
            std::cout << "--";
            std::cout << e.first;
            std::cout << ":";
            std::cout << e.second;
        }

        std::cout <<"\n";


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
        std::cout << "im ite" << std::endl;

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
        std::cout << "im antiv" << std::endl;

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
        std::cout << "im add" << std::endl;

        assert(f);
        assert(g);

        return antiv(f, g);
    }

    auto apply(std::int32_t const w, std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {
        std::cout << "im apply" << std::endl;

        assert(f);

        if (f == tmls[2] || f == tmls[3]){ //tmls[2] = -10 -> w=0 | tmls[3] = -11 -> w=1
            std::cout << "im apply EXPANSION" << std::endl;

            return foa(std::make_shared<detail::edge>(((f->w == -11 && w == 0) || (f->w == -10 && w == 1)) ? -11 : -10, f->v));
        }


        return foa(std::make_shared<detail::edge>(((f->w == 1 && w == 0) || (f->w == 0 && w == 1)) ? 1 : 0, f->v));
    }

    auto complement(std::shared_ptr<detail::edge> const& f) -> std::shared_ptr<detail::edge> override
    {

        assert(f);

        if (f == tmls[2]){
            std::cout << "im complement EXPANSION 0" << std::endl;
            return tmls[3];
        }
        if (f == tmls[3]){
            std::cout << "im complement EXPANSION 1" << std::endl;
            return tmls[2];
        }

        return ((f->w == 0) ? foa(std::make_shared<detail::edge>(1, f->v))
                            : foa(std::make_shared<detail::edge>(0, f->v)));
    }

    auto conj(std::shared_ptr<detail::edge> f, std::shared_ptr<detail::edge> g)
        -> std::shared_ptr<detail::edge> override
    {
        std::cout << "im conj" << std::endl;


        assert(f);
        assert(g);





        if ((f == tmls[2] || f == tmls[3]) && (g == tmls[2] || g == tmls[3])){
            std::cout << "beide expansion" << std::endl;
            return tmls[2];
        }

        if (f == tmls[2] || f == tmls[3]){
            std::cout << "f ist expansion" << std::endl;

            return replaceOnesWithExp(g, false, f);
        }
        if (g == tmls[2] || g == tmls[3]){
            std::cout << "g ist expansion" << std::endl;




            return replaceOnesWithExp(f, false, g);
        }

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



        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, 100);
        int randomNumber = dist(gen);


        std::shared_ptr<detail::edge> r = f;




        //normal
        if (randomNumber <= 100) {
            //std::cout << "1\n";


            std::shared_ptr<detail::edge> conj2;

            auto cof4 = cof(g, x, false);
            if (cof4->w < 0 && (!f->v || f->v->x != x)){
                conj2 = cof4;
            } else {
                auto cof3 = cof(f, x, false);

                if (cof3->w < 0 && (!g->v || g->v->x != x)){
                    conj2 = cof3;
                } else {
                    conj2 = conj(cof3, cof4);
                }
            }


            std::shared_ptr<detail::edge> conj1;

            auto cof2 = cof(g, x, true);
            if (cof2->w < 0 && (!f->v || f->v->x != x)){

                conj1 = cof2;
            } else {
                auto cof1 = cof(f, x, true);
                if (cof1->w < 0 && (!g->v || g->v->x != x)){
                    conj1 = cof1;
                } else {
                    conj1 = conj(cof1, cof2);
                }
            }

            r = make_branch(x, conj1, conj2);

            //r = make_branch(x, conj(cof(f, x, true), cof(g, x, true)), conj(cof(f, x, false), cof(g, x, false)));

        }

        //lo becomes extension
        else if (randomNumber <= 100){
            /*
            std::cout << "2\n";

             std::shared_ptr<detail::edge> conj1;

            auto cof2 = cof(g, x, true);
            if (cof2->w < 0 && (!f->v || f->v->x != x)){

                conj1 = cof2;
            } else {
                auto cof1 = cof(f, x, true);
                if (cof1->w < 0 && (!g->v || g->v->x != x)){
                    conj1 = cof1;
                } else {
                    conj1 = conj(cof1, cof2);
                }
            }

            r = make_branch(x, conj1, tmls[2]);
             */

            r = make_branch(x, conj(cof(f, x, true), cof(g, x, true)), tmls[2]);


        // hi becomes extension
        } else {
            std::cout << "3\n";

            /*
            std::shared_ptr<detail::edge> conj2;

            auto cof4 = cof(g, x, false);
            if (cof4->w < 0 && (!f->v || f->v->x != x)){
                conj2 = cof4;
            } else {
                auto cof3 = cof(f, x, false);

                if (cof3->w < 0 && (!g->v || g->v->x != x)){
                    conj2 = cof3;
                } else {
                    conj2 = conj(cof3, cof4);
                }
            }
            r = make_branch(x, tmls[2], conj2);
             */

            r = make_branch(x, tmls[2], conj(cof(f, x, false), cof(g, x, false)));
        }



        ct.insert_or_assign({operation::AND, f, g}, std::make_pair(r, 0.0));

        return r;
    }


    std::shared_ptr<detail::edge> replaceOnesWithExp(std::shared_ptr<detail::edge> f, bool goesTrue, std::shared_ptr<detail::edge> ex) {
        assert(f);
        assert(ex);

        if (f == tmls[2] || f == tmls[3]){
            return f;
        }

        if (goesTrue){
            if (f == tmls[0]){
                if (ex == tmls[2]){
                    return tmls[3];
                }
                if (ex == tmls[3]){
                    return tmls[2];
                }
            }
            if (f == tmls[1]){
                return f;
            }
        } else {
            if (f == tmls[0]){
                return f;
            }
            if (f == tmls[1]){
                return ex;
            }
        }

        if (f->w == 0) {
            f->v->hi = replaceOnesWithExp(f->v->hi, goesTrue, ex);
            f->v->lo = replaceOnesWithExp(f->v->lo, goesTrue, ex);
        }

        if (f->w == 1){
            f->v->hi = replaceOnesWithExp(f->v->hi, !goesTrue, ex);
            f->v->lo = replaceOnesWithExp(f->v->lo, !goesTrue, ex);
        }

        return f;
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

        return (lo->w != 0 && lo->w != -10);
        //return (lo->w != 0 || lo->w != -1);
    }

    auto make_branch(std::int32_t const x, std::shared_ptr<detail::edge> hi, std::shared_ptr<detail::edge> lo)
        -> std::shared_ptr<detail::edge> override
    {
        std::cout << "making branch" << std::endl;

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

    friend bhd;
};

auto inline bhd::operator~() const
{
    assert(mgr);

    return bhd{mgr->complement(f), mgr};
}

auto inline bhd::operator&=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline bhd::operator|=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline bhd::operator^=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline bhd::high(bool const weighting) const
{
    assert(mgr);

    return bhd{mgr->high(f, weighting), mgr};
}

auto inline bhd::low(bool const weighting) const
{
    assert(mgr);

    return bhd{mgr->low(f, weighting), mgr};
}

template <typename T, typename... Ts>
auto inline bhd::cof(T const a, Ts... args) const
{
    assert(mgr);

    return bhd{mgr->subfunc(f, a, std::forward<Ts>(args)...), mgr};
}

template <typename T, typename... Ts>
auto inline bhd::eval(T const a, Ts... args) const
{
    assert(mgr);

    return mgr->eval(f, a, std::forward<Ts>(args)...);
}

auto inline bhd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bhd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bhd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bhd::is_essential(std::int32_t const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline bhd::is_zero() const noexcept
{
    assert(mgr);

    return (*this == mgr->zero());
}

auto inline bhd::is_one() const noexcept
{
    assert(mgr);

    return (*this == mgr->one());
}

auto inline bhd::ite(bhd const& g, bhd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);  // transitive property

    return bhd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline bhd::compose(std::int32_t const x, bhd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bhd{mgr->compose(f, x, g.f), mgr};
}

auto inline bhd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bhd{mgr->restr(f, x, a), mgr};
}

auto inline bhd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->exist(f, x), mgr};
}

auto inline bhd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->forall(f, x), mgr};
}

auto inline bhd::satcount() const
{
    assert(mgr);

    return mgr->satcount(f);
}

auto inline bhd::print() const
{
    assert(mgr);

    mgr->print({*this});
}
}  // namespace freddy::dd
