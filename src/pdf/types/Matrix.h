#pragma once

struct Matrix
{
    double a{ 0.0 }, b{ 0.0 }, c{ 0.0 }, d{ 0.0 }, x{ 0.0 }, y{ 0.0 };

    void ToIdentity()
    {
        a = d = 1.0;
        b = c = 0.0;
        x = y = 0.0;
    }

    static Matrix IdentityMatrix() {
        Matrix a;
        a.ToIdentity();
        return a;
    }
};

void Multiply(Matrix& current, const Matrix& new_value);