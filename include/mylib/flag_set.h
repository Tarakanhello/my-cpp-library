#ifndef FLAG_SET_H
#define FLAG_SET_H

#include "mylib/bit_operations.h"
#include <format>

namespace mylib
{

    /**
     * @brief Класс для управления набором логических флагов (битовых флагов).
     *
     * Хранит до 64 флагов в одном 64‑битном целом числе.
     * Предоставляет удобные методы для установки, сброса, проверки и инвертирования
     * отдельных флагов, а также для пакетных операций и сериализации.
     *
     * @note Индексы флагов начинаются с 0 (младший бит).
     * @throws std::out_of_range при попытке обратиться к индексу вне допустимого диапазона.
     */
    class FlagSet final
    {
    private:
        uint64_t m_flags{}; ///< Хранилище флагов (битовая маска).
        int m_numFlags{};   ///< Количество флагов (задаётся в конструкторе).

        /**
         * @brief Проверяет, находится ли индекс в допустимом диапазоне.
         *
         * @param index Проверяемый индекс.
         * @param str Название метода, в котором происходит проверка (для сообщения об ошибке).
         * @throws std::out_of_range если индекс меньше 0 или не меньше m_numFlags.
         */
        void checkIndex(int index, std::string_view str) const
        {
            if(index < 0 || index >= m_numFlags)
            {
                throw std::out_of_range(std::format("mylib::FlagSet::{}: index is out of range", str));
            }
        }

    public:
        /**
         * @brief Конструктор, задающий количество флагов.
         *
         * @param numFlags Количество флагов (должно быть в диапазоне (0, 64]).
         * @throws std::out_of_range если numFlags <= 0 или numFlags > 64.
         */
        explicit FlagSet(int numFlags)
            : m_flags { bit::ZERO }
            , m_numFlags{ numFlags }
        {
            if(numFlags <= 0 || numFlags > 64)
            {
                throw std::out_of_range("mylib::FlagSet: numFlags must be in (0, 64]");
            }
            else
            {
                m_numFlags = numFlags;
            }
        }

        /**
         * @brief Устанавливает значение флага по индексу.
         *
         * @param index Индекс флага (0 … numFlags-1).
         * @param value Новое значение флага (true = 1, false = 0). По умолчанию true.
         * @throws std::out_of_range если индекс вне допустимого диапазона.
         */
        void set(int index, bool value = true)
        {
            checkIndex(index, "set");

            bit::set(m_flags, index, value);
        }

        /**
         * @brief Сбрасывает флаг (устанавливает в 0) по индексу.
         *
         * @param index Индекс флага (0 … numFlags-1).
         * @throws std::out_of_range если индекс вне допустимого диапазона.
         */
        void clear(int index)
        {
            checkIndex(index, "clear");

            set(index, false);
        }

        /**
         * @brief Инвертирует (переключает) флаг по индексу.
         *
         * @param index Индекс флага (0 … numFlags-1).
         * @throws std::out_of_range если индекс вне допустимого диапазона.
         */
        void toggle(int index)
        {
            checkIndex(index, "toggle");

            m_flags = bit::flip(m_flags, index);
        }

        /**
         * @brief Проверяет состояние флага по индексу.
         *
         * @param index Индекс флага (0 … numFlags-1).
         * @return true если флаг установлен (1), иначе false.
         * @throws std::out_of_range если индекс вне допустимого диапазона.
         */
        bool test(int index) const
        {
            checkIndex(index, "test");

            return bit::get(m_flags, index);
        }

        /**
         * @brief Устанавливает все флаги в заданное значение.
         *
         * @param value Значение для всех флагов (true = 1, false = 0).
         */
        void setAll(bool value) noexcept
        {
            m_flags = value ? bit::lowerMask(m_numFlags) : bit::ZERO;
        }

        /**
         * @brief Сбрасывает все флаги (устанавливает все в 0).
         *
         * Эквивалентно вызову setAll(false).
         */
        void clearAll() noexcept
        {
            setAll(false);
        }

        /**
         * @brief Возвращает целочисленное представление всех флагов.
         *
         * @return uint64_t Биты от 0 до numFlags-1 содержат состояния флагов,
         *         остальные биты равны 0.
         */
        uint64_t toInteger() const
        {
            return bit::getValue(m_flags, 0, m_numFlags);
        }

        /**
         * @brief Устанавливает состояния флагов из целочисленного значения.
         *
         * Используются только младшие numFlags бит переданного числа.
         * Старшие биты игнорируются.
         *
         * @param value Целочисленное значение, содержащее состояния флагов.
         */
        void fromInteger(uint64_t value)
        {
            clearAll();
            bit::setValue(m_flags, value & bit::lowerMask(m_numFlags));
        }

        /**
         * @brief Сериализует флаги в массив из 8 байт (little‑endian порядок).
         *
         * @return std::array<uint8_t, 8> Массив байт, где байт 0 содержит младшие 8 бит,
         *         байт 7 – старшие.
         */
        std::array<uint8_t, 8> toBytes() const
        {
            std::array<uint8_t, 8> bytes{};

            for(int i{}; auto& byte : bytes)
            {
                byte = bit::getValue(m_flags, (i++) * 8, 8);
            }

            return bytes;
        }

        /**
         * @brief Восстанавливает состояния флагов из байтового массива (little‑endian).
         *
         * @param bytes Массив из 8 байт, где байт 0 соответствует младшим 8 битам.
         *         Байты за пределами numFlags бит игнорируются.
         */
        void fromBytes(const std::array<uint8_t, 8>& bytes)
        {
            for(int i{ 0 }; const auto& byte : bytes)
            {
                bit::setValue(m_flags, byte, (i++) * 8);
            }
        }

    };

} // end mylib namespace

#endif // FLAG_SET_H
