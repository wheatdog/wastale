#ifndef WASTALE_MATH_H
#define WASTALE_MATH_H

union v2
{
    struct
    {
        r32 X, Y;
    };
    r32 E[2];
};

inline v2
V2(r32 X, r32 Y)
{
    v2 Result;
    Result.X = X;
    Result.Y = Y;
    return Result;
}

union rect2
{
    struct
    {
        v2 Min, Max;
    };
    v2 E[2];
};

//
// NOTE(wheatdog): Scalar
//

inline r32
Square(r32 A)
{
    r32 Result = A * A;
    return Result;
}

//
// NOTE(wheatdog): v2
//

inline v2
operator-(v2 A)
{
    v2 Result;
    Result.X = -A.X;
    Result.Y = -A.Y;
    return Result;
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    return Result;
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    return Result;
}

inline v2
operator*(r32 A, v2 B)
{
    v2 Result;
    Result.X = A*B.X;
    Result.Y = A*B.Y;
    return Result;
}

inline v2
operator*(v2 A, r32 B)
{
    v2 Result = B * A;
    return Result;
}

inline v2 &
operator*=(v2 &A, r32 B)
{
    A = A * B;
    return A;
}

inline v2 &
operator+=(v2 &A, v2 B)
{
    A = A + B;
    return A;
}

inline v2 &
operator-=(v2 &A, v2 B)
{
    A = A - B;
    return A;
}

inline r32
Dot(v2 A, v2 B)
{
    r32 Result;
    Result = A.X*B.X + A.Y*B.Y;
    return Result;
}

// NOTE(wheatdog): (A x B) x C = B(C . A) - A(C . B)
inline v2
VectorTripleProduct(v2 A, v2 B, v2 C)
{
    v2 Result = Dot(A, C)*B - Dot(C, B)*A;
    return Result;
}

//
// NOTE(wheatdog): rect2
//

inline rect2
RectMinDim(v2 Min, v2 Dim)
{
    rect2 Result;
    Result.Min = Min;
    Result.Max = Min + Dim;
    return Result;
}

#endif
