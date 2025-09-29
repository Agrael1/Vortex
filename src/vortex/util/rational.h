#pragma once
#include <cstdint>
#include <concepts>
#include <expected>
#include <format>
#include <numeric>

namespace vortex {

// A simple rational number class to represent fractions, fully constexpr and noexcept.
template<std::integral T>
struct ratio {
    // Default constructor initializes to 0/1
    constexpr ratio() noexcept
        : numerator(0), denominator(1) { }
    constexpr ratio(T num, T denom = 1) noexcept
        : numerator(num), denominator(denom ? denom : 1)
    {
        normalize(); // Normalize the ratio on construction
    }
    template<std::integral U>
    constexpr ratio(const ratio<U>& other) noexcept
        : numerator(static_cast<T>(other.numerator)), denominator(static_cast<T>(other.denominator))
    {
        // Assuming the other ratio is already normalized, we can just copy the values
    }

public:
    constexpr ratio& operator=(T num) noexcept
    {
        numerator = num;
        denominator = 1;
        return *this;
    }
    constexpr ratio& assign(T num, T denom) noexcept
    {
        numerator = num;
        denominator = denom ? denom : 1;
        return *this;
    }
    constexpr T num() const noexcept
    {
        return numerator;
    }
    constexpr T denom() const noexcept
    {
        return denominator;
    }

    // arithmetic operators
    template<std::integral IntType = T>
    constexpr auto& operator+=(const ratio<IntType>& other)
    {
        // This calculation avoids overflow, and minimises the number of expensive
        // calculations. Thanks to Nickolay Mladenov for this algorithm.
        //
        // Proof:
        // We have to compute a/b + c/d, where gcd(a,b)=1 and gcd(b,c)=1.
        // Let g = gcd(b,d), and b = b1*g, d=d1*g. Then gcd(b1,d1)=1
        //
        // The result is (a*d1 + c*b1) / (b1*d1*g).
        // Now we have to normalize this ratio.
        // Let's assume h | gcd((a*d1 + c*b1), (b1*d1*g)), and h > 1
        // If h | b1 then gcd(h,d1)=1 and hence h|(a*d1+c*b1) => h|a.
        // But since gcd(a,b1)=1 we have h=1.
        // Similarly h|d1 leads to h=1.
        // So we have that h | gcd((a*d1 + c*b1) , (b1*d1*g)) => h|g
        // Finally we have gcd((a*d1 + c*b1), (b1*d1*g)) = gcd((a*d1 + c*b1), g)
        // Which proves that instead of normalizing the result, it is better to
        // divide num and den by gcd((a*d1 + c*b1), g)

        // Protect against self-modification
        IntType r_num = other.num();
        IntType r_den = other.denom();

        IntType g = std::gcd(denominator, r_den);
        denominator /= g; // = b1 from the calculations above
        numerator = numerator * (r_den / g) + r_num * denominator;
        g = std::gcd(numerator, g);
        numerator /= g;
        denominator *= r_den / g;

        return *this;
    }

    template<std::integral IntType = T>
    constexpr auto& operator-=(const ratio<IntType>& other)
    {
        // Similar to operator+=, but with subtraction
        return *this += ratio(-other.numerator, other.denominator);
    }

    template<std::integral IntType = T>
    constexpr ratio<IntType>& operator*=(const ratio<IntType>& r) noexcept
    {
        // Protect against self-modification
        IntType r_num = r.num();
        IntType r_den = r.denom();

        // Avoid overflow and preserve normalization
        IntType gcd1 = std::gcd(numerator, r_den);
        IntType gcd2 = std::gcd(r_num, denominator);
        numerator = (numerator / gcd1) * (r_num / gcd2);
        denominator = (denominator / gcd2) * (r_den / gcd1);
        return *this;
    }

    template<std::integral IntType = T>
    constexpr std::expected<ratio<IntType>, std::string_view> div(const ratio<IntType>& r) noexcept
    {
        // Protect against self-modification
        IntType r_num = r.num();
        IntType r_den = r.denom();

        // Avoid repeated construction
        IntType zero(0);

        // Trap division by zero
        if (r_num == zero) {
            return std::unexpected("Division by zero in ratio division");
        }
        if (numerator == zero) {
            return *this;
        }

        // Avoid overflow and preserve normalization
        IntType gcd1 = std::gcd(numerator, r_num);
        IntType gcd2 = std::gcd(r_den, denominator);
        numerator = (numerator / gcd1) * (r_den / gcd2);
        denominator = (denominator / gcd2) * (r_num / gcd1);

        if (denominator < zero) {
            numerator = -numerator;
            denominator = -denominator;
        }
        return *this;
    }

public:
    template<std::floating_point U = double>
    constexpr U value() const noexcept
    {
        return static_cast<U>(numerator) / denominator; // potential division by zero, but handled in operator/
    }

    constexpr void normalize() noexcept
    {
        if (denominator == 0) {
            return; // Avoid division by zero
        }
        if (numerator == 0) {
            denominator = 1; // Normalize zero to 0/1
            return;
        }
        // Reduce the fraction by finding the greatest common divisor (GCD)
        T gcd = std::gcd(numerator, denominator);

        if (gcd != 0) {
            numerator /= gcd;
            denominator /= gcd;
        }
        if (denominator < 0) {
            // Ensure the denominator is always positive
            numerator = -numerator;
            denominator = -denominator;
        }
    }

public:
    T numerator; // The numerator of the fraction
    T denominator; // The denominator of the fraction
};

template<std::integral IntType1, std::integral IntType2>
constexpr auto operator+(IntType1 one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result += other; // Use the += operator to perform the addition
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator-(IntType1 one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result -= other; // Use the -= operator to perform the subtraction
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator*(IntType1 one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result *= other; // Use the -= operator to perform the subtraction
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator/(IntType1 one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result.div(other); // Use the -= operator to perform the subtraction
}

template<std::integral IntType1, std::integral IntType2>
constexpr auto operator+(const ratio<IntType1>& one, IntType2 two) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the ratio to a common type
    return result += ratio<IntType2>(two); // Use the += operator to perform the addition
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator-(const ratio<IntType1>& one, IntType2 two) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the ratio to a common type
    return result -= ratio<IntType2>(two); // Use the -= operator to perform the subtraction
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator*(const ratio<IntType1>& one, IntType2 two) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the ratio to a common type
    return result *= ratio<IntType2>(two); // Use the -= operator to perform the subtraction
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator/(const ratio<IntType1>& one, IntType2 two) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the ratio to a common type
    return result.div(ratio<IntType2>(two)); // Use the -= operator to perform the subtraction
}

template<std::integral IntType1, std::integral IntType2>
constexpr auto operator+(const ratio<IntType1>& one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result += other; // Use the *= operator to perform the multiplication
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator-(const ratio<IntType1>& one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result -= other; // Use the *= operator to perform the multiplication
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator*(const ratio<IntType1>& one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result *= other; // Use the *= operator to perform the multiplication
}
template<std::integral IntType1, std::integral IntType2>
constexpr auto operator/(const ratio<IntType1>& one, const ratio<IntType2>& other) noexcept
{
    using ratio_type = ratio<std::common_type_t<IntType1, IntType2>>;
    ratio_type result(one); // Convert the integer to a ratio
    return result.div(other); // Use the div operator to perform the division
}

using ratio32_t = ratio<int32_t>;
using ratio64_t = ratio<int64_t>;
} // namespace vortex

namespace std {
// Formatter specialization for vortex::ratio
template<>
struct formatter<vortex::ratio<int32_t>> {
    // Basic parse that simply returns the end of the format string
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    // Format function that outputs ratio fields
    template<typename FormatContext>
    auto format(const vortex::ratio<int32_t>& r, FormatContext& ctx) const {
        return format_to(ctx.out(), "{}/{}", r.num(), r.denom());
    }
};
} // namespace std
