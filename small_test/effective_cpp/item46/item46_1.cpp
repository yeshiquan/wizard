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
    const Rational operator*(const Rational& lhs, const Rational& rhs);
private:
    T num;
    T deno;
};

template<typename T>
const Rational<T> operator*(const Rational<T>& lhs, const Rational<T>& rhs) {
    return Rational<T>(lhs.numerator() * rhs.numerator(), lhs.denominator() * rhs.denominator());
}

int main() {
    Rational<int> one_half(1, 2);
    Rational<int> result = one_half * 2;
}
