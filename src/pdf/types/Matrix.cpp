#include "Matrix.h"

void Multiply(Matrix& current, const Matrix& new_value)
{
    double _11, _12, _21, _22, _31, _32;

    _11 = current.a * new_value.a + current.b * new_value.c;
    _12 = current.a * new_value.b + current.b * new_value.d;
    _21 = current.c * new_value.a + current.d * new_value.c;
    _22 = current.c * new_value.b + current.d * new_value.d;
    _31 = current.x * new_value.a + current.y * new_value.c + 1.0 * new_value.x;
    _32 = current.x * new_value.b + current.y * new_value.d + 1.0 * new_value.y;


    /*_11 = new_value.a * current.a + new_value.b * current.c + 0.0 * current.x;
    _12 = new_value.a * current.b + new_value.b * current.d + 0.0 * current.y;
    _21 = new_value.c * current.a + new_value.d * current.c + 0.0 * current.x;
    _22 = new_value.c * current.b + new_value.d * current.d + 0.0 * current.y;
    _31 = new_value.x * current.a + new_value.y * current.c + 1.0 * current.x;
    _32 = new_value.x * current.b + new_value.y * current.d + 1.0 * current.y;*/

    current.a = _11;
    current.b = _12;
    current.c = _21;
    current.d = _22;
    current.x = _31;
    current.y = _32;
}