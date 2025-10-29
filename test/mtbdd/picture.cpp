#include "catch2/catch_test_macros.hpp"
#include "freddy/dd/mtbdd.hpp"

#include <iostream>
#include <vector>
#include <filesystem>

using namespace freddy;

auto create_vars(int& amount, dd::mtbdd_manager<std::int32_t>& mgr){
    for (auto i = mgr.var_count(); i < amount; ++i)
    {
        mgr.var();
    }
}

auto operate_and(freddy::dd::mtbdd<int>& f, std::vector<bool>& bits, dd::mtbdd_manager<std::int32_t>& mgr){
    for (int i = 0; i < bits.size(); ++i) {
        if (bits[i]){
            f &= mgr.var(i);
        } else {
            f &= ~mgr.var(i);
        }
    }
}

auto int_to_bits(int x, int size){
    std::vector<bool> bits(size, false);

    for (int i = 0; i < size; ++i) {
        bits[size - 1 - i] = (x >> i) & 1;
    }

    return bits;
}

auto matrix_img_to_add(std::vector<std::vector<int>>& img, int& height, int& width, dd::mtbdd_manager<std::int32_t>& mgr) {
    const int height_vars = std::ceil(std::log2(height));
    const int width_vars = std::ceil(std::log2(width));
    int var_amount = height_vars + width_vars;
    create_vars(var_amount, mgr);

    freddy::dd::mtbdd<int> add = mgr.zero();

    const int matrix_height = std::pow(2, height_vars);
    const int matrix_width = std::pow(2, width_vars);
    int pixel = 0;
    for (int y = 0; y < matrix_height; ++y) {
        for (int x = 0; x < matrix_width; ++x) {
            if (y < height && x < width){
                std::vector<bool> bits = int_to_bits(pixel, var_amount);

                auto conj = mgr.constant(img[y][x]);
                operate_and(conj, bits, mgr);
                add |= conj;
            }
            pixel++;
        }
    }
    return add;
}

auto matrix_add_to_img(freddy::dd::mtbdd<int>& adds, int& height, int& width){
    const int height_vars = std::ceil(std::log2(height));
    const int width_vars = std::ceil(std::log2(width));
    const int var_amount = height_vars + width_vars;

    std::vector<std::vector<int>> new_img (height, std::vector<int>(width, 0));

    int pixel = 0;
    const int matrix_height = std::pow(2, height_vars);
    const int matrix_width = std::pow(2, width_vars);
    for (int y = 0; y < matrix_height; ++y) {
        for (int x = 0; x < matrix_width; ++x) {
            if (y < height && x < width){
                std::vector<bool> bits = int_to_bits(pixel, var_amount);
                new_img[y][x] = adds.eval(bits);
            }
            pixel++;
        }
    }

    return new_img;
}

TEST_CASE("pictures are compressible", "[basic]")
{
    dd::mtbdd_manager<std::int32_t> mgr;

    std::vector<std::vector<int>> img = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 128, 0, 0, 128, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 128, 128, 255, 255, 128, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 30, 128, 255, 77, 255, 128, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 255, 255, 255, 255, 255, 128, 128, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 85, 68, 85, 255, 128, 128, 85, 30, 30, 30, 30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 128, 255, 255, 128, 85, 30, 30, 30, 30, 30, 30, 30, 30, 30, 128, 30, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 255, 128, 85, 30, 30, 30, 128, 255, 128, 30, 30, 30, 128, 0, 128, 128, 30, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 128, 255, 128, 128, 255, 255, 255, 255, 128, 128, 128, 0, 128, 0, 255, 128, 30, 30, 30, 0 },
        { 0, 0, 0, 0, 128, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 128, 0, 0, 0, 255, 255, 128, 0, 0 },
        { 0, 0, 0, 0, 53, 128, 255, 255, 255, 128, 53, 128, 255, 255, 255, 128, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 128, 53, 53, 255, 255, 0, 53, 53, 128, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 0, 53, 53, 0, 255, 128, 0, 0, 128, 53, 0, 255, 128, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 128, 53, 0, 128, 255, 0, 0, 128, 53, 0, 128, 255, 128, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 0, 0, 53, 53, 0, 128, 128, 0, 0, 53, 53, 0, 255, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    int height = img.size();
    int width = img[0].size();

    auto adds = matrix_img_to_add(img, height, width, mgr);

    auto new_img = matrix_add_to_img(adds, height, width);

    CHECK(img == new_img);
}