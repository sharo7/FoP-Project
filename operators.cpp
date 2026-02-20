#include "operators.h"

template<typename T>
T addition(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    return a + b;
}

template<typename T>
T subtraction(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    return a - b;
}

template<typename T>
T multiplication(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    return a * b;
}

template<typename T>
T division(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    if (b == 0)
        throw invalid_argument("division by zero");
    return a / b;
}

template<typename T>
T myModulus(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    if (b == 0)
        throw invalid_argument("division by zero");
    return a % b;
}

template<typename T>
bool isEqual(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    if constexpr (is_floating_point<T>::value)
        return fabs(a - b) < 1e-9;
    return a == b;
}

template<typename T>
bool isGreaterThan(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    if constexpr (is_floating_point<T>::value)
        return a - b > 1e-9;
    return a > b;
}

template<typename T>
bool isLessThan(T a, T b) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    if constexpr (is_floating_point<T>::value)
        return (b - a) > 1e-9;
    return a < b;
}

template<typename T>
T myAbs(T a) {
    static_assert(is_arithmetic<T>::value, "input must be arithmetic");
    return a < 0 ? -a : a;
}

double mySqrt(double a) {
    if (a < 0)
        throw domain_error("sqrt: negative input");
    return sqrt(a);
}

double myFloor(double a) {
    return floor(a);
}

double myCeil(double a) {
    return ceil(a);
}

double mySinus(double a) {
    return sin(a);
}

double myCosine(double a) {
    return cos(a);
}

template<typename T>
bool myAnd(T a, T b) {
    static_assert(is_same<T, bool>::value, "Input must be bool");
    return a && b;
}

template<typename T>
bool myOr(T a, T b) {
    static_assert(is_same<T, bool>::value, "Input must be bool");
    return a || b;
}

template<typename T>
bool myNot(T a) {
    static_assert(is_same<T, bool>::value, "Input must be bool");
    return !a;
}

template<typename T>
bool myXor(T a, T b) {
    static_assert(is_same<T, bool>::value, "Input must be bool");
    if ((a && !b) || (!a && b))
        return true;
    return false;
}

template<typename T>
size_t lengthOfString(const T &s) {
    static_assert(is_same<T, string>::value, "Input must be string");
    return s.size();
}

char charAt(int n, const string &s) {
    if (n > 0 && static_cast<size_t>(n) <= s.length())
        return s[n - 1];
    return '\0';
}

template<typename T1, typename T2>
string stringConcat(const T1 &s1, const T2 &s2) {
    static_assert(is_same<T1, string>::value && is_same<T2, string>::value, "Both inputs must be string");
    return s1 + s2;
}
