#ifndef SCRATCH_OPERATORS_H
#define SCRATCH_OPERATORS_H

#include <bits/stdc++.h>

using namespace std;

template<typename T>
T addition(T a, T b);

template<typename T>
T subtraction(T a, T b);

template<typename T>
T multiplication(T a, T b);

template<typename T>
T division(T a, T b);

template<typename T>
T myModulus(T a, T b);

template<typename T>
bool isEqual(T a, T b);

template<typename T>
bool isGreaterThan(T a, T b);

template<typename T>
bool isLessThan(T a, T b);

template<typename T>
T myAbs(T a);

double mySqrt(double a);

double myFloor(double a);

double myCeil(double a);

double mySinus(double a);

double myCosine(double a);

template<typename T>
bool myAnd(T a, T b);

template<typename T>
bool myOr(T a, T b);

template<typename T>
bool myNot(T a);

template<typename T>
bool myXor(T a, T b);

template<typename T>
size_t lengthOfString(const T &s);

char charAt(int n, const string &s);

template<typename T1, typename T2>
string stringConcat(const T1 &s1, const T2 &s2);

#endif //SCRATCH_OPERATORS_H
