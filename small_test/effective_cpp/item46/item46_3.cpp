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
    // operator*不做任何事情，只调用定义在类外面的一个辅助函数
    const Rational operator*(const Rational& lhs, const Rational& rhs) {
        return multiply(lhs, rhs);
    }
private:
    T num;
    T deno;
};

template<typename T>
const Rational<T> multiply(const Rational<T>& lhs, const Rational<T>& rhs) {
    return Rational<T>(lhs.numerator() * rhs.numerator(), lhs.denominator() * rhs.denominator());
}

int main() {
    Rational<int> one_half(1, 2);
    Rational<int> result = one_half * 2;
}
