#include <iostream>

template<typename T>
class Rational {
public:
    Rational(const T& numerator = 0, const T& denominator = 1) {
    }
    const T numerator() const {
        return num;
    }
    const T denominator() const {
        return deno;
    }
friend
    const Rational operator*(const Rational& lhs, const Rational& rhs) {
        return Rational<T>(lhs.numerator() * rhs.numerator(), lhs.denominator() * rhs.denominator());
    }
private:
    T num;
    T deno;
};

int main() {
    Rational<int> one_half(1, 2);
    Rational<int> result = one_half * 2;
}
