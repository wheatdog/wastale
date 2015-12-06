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

union v3
{
    struct
    {
        r32 X, Y, Z;
    };
    r32 E[3];
};

inline v3
V3(r32 X, r32 Y, r32 Z)
{
    v3 Result;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
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

struct line2
{
    v2 Start;
    v2 Direction;
    r32 Length;
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

inline v3
Cross(v2 A, v2 B)
{
    v3 Result = V3(0, 0, A.X*B.Y-A.Y*B.X);
    return Result;
}

// NOTE(wheatdog): (A x B) x C = B(C . A) - A(C . B)
inline v2
VectorTripleProduct(v2 A, v2 B, v2 C)
{
    v2 Result = Dot(A, C)*B - Dot(C, B)*A;
    return Result;
}

inline r32
Length2(v2 A)
{
    r32 Result = Dot(A, A);
    return Result;
}

inline r32
Length(v2 A)
{
    r32 Result = sqrtf(Dot(A, A));

    return Result;
}

// TODO(wheatdog): Do I want to check whether length is zero here?
inline v2
Normalize(v2 A)
{
    v2 Result = A*(1.0f / Length(A));
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

inline v2
GetWidthDirection(rect2 Rect)
{
    v2 Result = V2(Rect.Max.X - Rect.Min.X, 0);
    return Result;
}

inline v2
GetHeightDirection(rect2 Rect)
{
    v2 Result = V2(0, Rect.Max.Y - Rect.Min.Y);
    return Result;
}

//
// NOTE(wheatdog): line
//

inline line2
LineStartDim(v2 Start, v2 Direction)
{
    line2 Result;
    Result.Start = Start;
    Result.Direction = Direction;
    Result.Length = Length(Direction);
    return Result;
}

#endif
