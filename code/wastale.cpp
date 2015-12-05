#include <math.h>
#include "wastale.h"

internal void
FillSoundOutput(game_sound *GameSound, game_state *GameState)
{
    u32 WavePeriod = (u32)(GameSound->SamplePerSecond / GameState->ToneHz);
    i16 *SampleOut = (i16 *)GameSound->Buffer;
    for (u32 SampleIndex = 0;
         SampleIndex < GameSound->SampleCount;
         ++SampleIndex)
    {
        i16 SampleValue = (i16)(GameState->Volume * sinf(GameState->tSine));

        for (u32 ChannelIndex = 0;
             ChannelIndex < GameSound->ChannelCount;
             ++ChannelIndex)
        {
            *SampleOut++ = SampleValue;
        }

        GameState->tSine += 2.0f*Pi32*(1.0f / (r32)WavePeriod);
        if (GameState->tSine > 2.0f*Pi32)
        {
            GameState->tSine -= 2.0f*Pi32;
        }
    }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, i32 XOffset, i32 YOffset)
{
    u8 *Row = (u8 *)Buffer->Memory;
    for (i32 Y = 0; Y < Buffer->Height; ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        // u8 *Pixel = Row;
        for (i32 X = 0; X < Buffer->Width; ++X)
        {
            // 0xAARRGGBB
            u8 R = (u8)(X + XOffset);
            u8 G = 255;
            u8 B = (u8)(Y + YOffset);
            *Pixel++ = B|(G << 8)|(R << 16);
        }
        Row += Buffer->PitchInByte;
    }
}

internal void
DrawRect(game_offscreen_buffer *Buffer,
         r32 Real32X, r32 Real32Y, r32 Real32Width, r32 Real32Height,
         r32 Real32R, r32 Real32G, r32 Real32B)
{
    i32 X = RoundReal32ToInt32(Real32X);
    i32 Y = RoundReal32ToInt32(Real32Y);
    i32 Width = RoundReal32ToInt32(Real32Width);
    i32 Height = RoundReal32ToInt32(Real32Height);

    i32 MinX = (X < 0)? 0 : X;
    i32 MaxX = X + Width;
    if (MaxX < 0) MaxX = 0;
    if (MaxX > Buffer->Width) MaxX = Buffer->Width;

    i32 MinY = (Y < 0)? 0 : Y;
    i32 MaxY = Y + Height;
    if (MaxY < 0) MaxY = 0;
    if (MaxY > Buffer->Height) MaxY = Buffer->Height;

    u8 R = (u8)(Real32R*255.0f);
    u8 G = (u8)(Real32G*255.0f);
    u8 B = (u8)(Real32B*255.0f);
    u32 Color = B|(G << 8)|(R << 16);

    u8 *Start = (u8 *)Buffer->Memory + MinY*Buffer->PitchInByte + MinX*Buffer->BytePerPixel;
    for (i32 YIndex = MinY; YIndex < MaxY; ++YIndex)
    {
        u32 *Pixel = (u32 *)Start;
        for (i32 XIndex = MinX; XIndex < MaxX; ++XIndex)
        {
            *Pixel++ = Color;
        }
        Start += Buffer->PitchInByte;
    }
}

internal u32
AddEntity(game_state *GameState, v2 P, v2 WidthHeight, entity_type Type)
{
    u32 EntityIndex = GameState->EntityCount++;

    entity *Entity = GameState->Entities + EntityIndex;

    Entity->Exist = true;
    Entity->P = P;
    Entity->WidthHeight = WidthHeight;
    Entity->Type = Type;

    return EntityIndex;
}

inline v2
GetFarthestPointInDimension(rect2 Rect, v2 Dim)
{
    v2 Point[4];
    Point[0] = Rect.Min;
    Point[1] = Rect.Max;
    Point[2] = V2(Rect.Min.X, Rect.Max.Y);
    Point[3] = V2(Rect.Max.X, Rect.Min.Y);
    r32 FarthestDotValue = Dot(Point[0], Dim);
    u32 FarthestIndex = 0;
    for (u32 Index = 1; Index < ArrayCount(Point); ++Index)
    {
        r32 Value = Dot(Point[Index], Dim);
        if ((Value - FarthestDotValue) > 0.0001)
        {
            FarthestDotValue = Value;
            FarthestIndex = Index;
        }
    }

    return Point[FarthestIndex];
}

internal v2
Support(rect2 A, rect2 B, v2 Dim)
{
    v2 Point1 = GetFarthestPointInDimension(A, Dim);
    v2 Point2 = GetFarthestPointInDimension(B, -Dim);

    v2 Result = Point1 - Point2;

    return Result;
}

// TODO(wheatdog): Now this simplex is for 2D collision detection
struct simplex
{
    u32 VertexCount;
    v2 Vertex[3];
};

struct simplex_check_result
{
    b32 ContainOrigin;
    v2 NextDim;
};

inline simplex_check_result
CheckSimplex(simplex *Simplex)
{
    simplex_check_result Result = {};

    v2 AO = -Simplex->Vertex[Simplex->VertexCount-1];

    if (Simplex->VertexCount == 2)
    {
        v2 AB = Simplex->Vertex[0] - Simplex->Vertex[1];
        Result.NextDim = VectorTripleProduct(AB, AO, AB);
        return Result;
    }

    Assert(Simplex->VertexCount == 3);

    v2 AB = Simplex->Vertex[1] - Simplex->Vertex[2];
    v2 AC = Simplex->Vertex[0] - Simplex->Vertex[2];

    v2 ABPerp = VectorTripleProduct(AC, AB, AB);
    if (Dot(ABPerp, AO) > 0)
    {
        Simplex[0] = Simplex[2];
        Simplex->VertexCount = 2;

        Result.NextDim = ABPerp;
        return Result;
    }

    v2 ACPerp = VectorTripleProduct(AB, AC, AC);
    if (Dot(ACPerp, AO) > 0)
    {
        Simplex[1] = Simplex[2];
        Simplex->VertexCount = 2;

        Result.NextDim = ACPerp;
        return Result;
    }

    Result.ContainOrigin = true;
    return Result;
}

internal b32
GJKCollisionDetction(rect2 A, rect2 B)
{
    simplex Simplex = {};

    // TODO(wheatdog): Find a better start point.
    v2 Dim = A.Max;
    Simplex.Vertex[Simplex.VertexCount++] = Support(A, B, Dim);
    Dim = -Dim;

    while(true)
    {
        u32 LastIndex = Simplex.VertexCount;

        Simplex.Vertex[Simplex.VertexCount++] = Support(A, B, Dim);

        if (Dot(Simplex.Vertex[LastIndex], Dim) <= 0.0f)
        {
            return false;
        }

        simplex_check_result Result = CheckSimplex(&Simplex);
        if (Result.ContainOrigin)
        {
            return true;
        }

        Dim = Result.NextDim;
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    game_state *GameState = (game_state *)GameMemory->PermanentStorage;
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);
    if (!GameMemory->IsInitial)
    {
        // NOTE(wheatdog): Index Zero means this entity isn't existed
        GameState->EntityCount = 1;

        v2 PlatformXY = V2(5.0f, 5.0f);
        v2 PlatformWH = V2(10.0f, 1.5f);
        AddEntity(GameState, PlatformXY, PlatformWH, EntityType_Wall);

        v2 GroundXY = V2(0.0f, 0.0f);
        v2 GroundWH = V2(30.0f, 1.5f);
        AddEntity(GameState, GroundXY, GroundWH, EntityType_Wall);

        GameState->MeterPerPixel = 30;

        GameMemory->IsInitial = true;
    }

    for (u32 ControllerIndex = 0;
         ControllerIndex < ArrayCount(GameInput->Controllers);
         ++ControllerIndex)
    {
        game_controller_input *Controller = GameInput->Controllers + ControllerIndex;

        if (!Controller->IsConnected)
        {
            continue;
        }

        u32 EntityIndex = GameState->ControllerToEntityIndex[ControllerIndex];
        if (!EntityIndex)
        {
            if (!Controller->Start.EndedDown)
            {
                continue;
            }
            EntityIndex = AddEntity(GameState, V2(0.0f, 0.0f), V2(1.0f, 1.5f), EntityType_Player);
            GameState->ControllerToEntityIndex[ControllerIndex] = EntityIndex;
        }

        r32 PlayerAccelerate = 50.0f; // m/s^2
        v2 Gravity = V2(0.0f, 0.0f); //V2(0.0f, -9.8f);
        r32 Drag = 5.0f;

        entity *Entity = GameState->Entities + EntityIndex;
        Entity->ddP = V2(Controller->LeftStick.X, Controller->LeftStick.Y)*PlayerAccelerate - Entity->dP*Drag + Gravity;
        Entity->dP += Entity->ddP*GameInput->dtForFrame;
        Entity->P += Entity->dP*GameInput->dtForFrame + 0.5*Entity->ddP*Square(GameInput->dtForFrame);


        for (u32 TestEntityIndex = 1;
             TestEntityIndex < GameState->EntityCount;
             ++TestEntityIndex)
        {
            if (TestEntityIndex == EntityIndex)
            {
                continue;
            }

            entity *TestEntity = GameState->Entities + TestEntityIndex;
            if (!TestEntity->Exist)
            {
                continue;
            }

            TestEntity->Collide = false;
            if (GJKCollisionDetction(RectMinDim(TestEntity->P, TestEntity->WidthHeight),
                                     RectMinDim(Entity->P, Entity->WidthHeight)))
            {
                TestEntity->Collide = true;
            }
        }
    }

    // NOTE(wheatdog): Clear screen to black
    DrawRect(Buffer, 0, 0, (r32)Buffer->Width, (r32)Buffer->Height, 0.0f, 0.0f, 0.0f);

    for (u32 EntityIndex = 1;
         EntityIndex < GameState->EntityCount;
         ++EntityIndex)
    {
        entity *Entity = GameState->Entities + EntityIndex;

        if (!Entity->Exist)
        {
            continue;
        }

        switch(Entity->Type)
        {
            case EntityType_Player:
            {
                DrawRect(Buffer,
                         (Entity->P.X*GameState->MeterPerPixel),
                         (Entity->P.Y*GameState->MeterPerPixel),
                         (Entity->WidthHeight.X*GameState->MeterPerPixel),
                         (Entity->WidthHeight.Y*GameState->MeterPerPixel),
                         1.0f, 0.0f, 0.0f);
            } break;

            case EntityType_Wall:
            {
                r32 Blue = 0.0f;
                if (Entity->Collide)
                {
                    Blue = 1.0f;
                }

                DrawRect(Buffer,
                         (Entity->P.X*GameState->MeterPerPixel),
                         (Entity->P.Y*GameState->MeterPerPixel),
                         (Entity->WidthHeight.X*GameState->MeterPerPixel),
                         (Entity->WidthHeight.Y*GameState->MeterPerPixel),
                         1.0f, 1.0f, Blue);
            } break;
        }
    }
}

//NOTE(wheatdog): This must be fast. ~1ms
GAME_FILL_SOUND(GameFillSound)
{
    game_state *GameState = (game_state *)GameMemory->PermanentStorage;
    if (!GameMemory->IsInitial)
    {
        GameState->ToneHz = 512;
        GameState->Volume = 0;
    }

    FillSoundOutput(GameSound, GameState);
}
