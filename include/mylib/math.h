#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#include <stdexcept>

namespace math
{
    /**
     * @brief Вычисляет частное от деления с округлением вверх.
     *
     * Возвращает наименьшее целое число, которое больше или равно
     * \f$ \frac{\text{dividend}}{\text{divisor}} \f$.
     *
     * @param dividend Делимое (беззнаковое 64-битное).
     * @param divisor  Делитель (знаковый 64-битный). Должен быть > 0.
     *
     * @return Результат деления с округлением вверх в виде знакового 64-битного числа.
     *
     * @throws std::invalid_argument Если divisor == 0.
     * @warning Поведение не определено, если divisor < 0.
     *
     * \code{.cpp}
     * math::ceiling(10, 3) == 4   // 10/3 ≈ 3.33 → 4
     * math::ceiling(8, 2)  == 4   // 8/2 = 4 → 4
     * \endcode
     */
    std::int64_t ceiling(std::uint64_t dividend, std::int64_t divisor)
    {
        if(0 == divisor)
        {
            throw std::invalid_argument("division by zero");
        }

        return static_cast<std::int64_t>(dividend / divisor) +
               static_cast<std::int64_t>(dividend % divisor != 0);
    }
} // end namespace math

#endif // MATH_H_INCLUDED
