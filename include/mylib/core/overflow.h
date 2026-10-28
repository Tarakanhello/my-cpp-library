#ifndef OVERFLOW_H
#define OVERFLOW_H

#include <concepts>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace mylib
{

    /** @brief Тег, выбирающий вид проверки. */
    enum class OverflowOp { Add, Mul };

    /**
     * @brief Бросает std::length_error, если a + b (или a * b) переполняет T.
     * @tparam op  Вид операции (Add / Mul).
     * @tparam T   Беззнаковый целочисленный тип.
     * @param a, b Операнды.
     * @param what Контекст для диагностики (например, "Bitset::appendLSB(...)").
     */
    template<OverflowOp op, std::unsigned_integral UINT>
    constexpr void checkOverflow(UINT a, UINT b, std::string_view what = "")
    {
        constexpr UINT max{ std::numeric_limits<UINT>::max()} ;
        if constexpr(op == OverflowOp::Add)
        {
            if(a > max - b)
            {
                throw std::length_error(std::format("{}: size overflow", what));
            }
        }
        else if constexpr(op == OverflowOp::Mul)
        {
            if(b != 0 && a > max / b)
            {
                throw std::length_error(std::format("{}: size overflow", what));
            }
        }
    }


    template<std::unsigned_integral UINT>
    constexpr void checkAddOverflow(UINT a, UINT b, std::string_view what = "")
    {
        checkOverflow<OverflowOp::Add>(a, b, what);
    }


    template<std::unsigned_integral UINT>
    constexpr void checkMulOverflow(UINT a, UINT b, std::string_view what = "")
    {
        checkOverflow<OverflowOp::Mul>(a, b, what);
    }



} // end mylib namespace

#endif // OVERFLOW_H
