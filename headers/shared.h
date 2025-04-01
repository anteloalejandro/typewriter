#ifndef SHARED
#define SHARED

#include "../raylib/include/raylib.h"

typedef struct {
    int x, y;
} Position;

typedef struct {
    int x, y;
    Vector2 a, b, c;
} Triangle;

extern struct Data {
    Font font;
    int fontSize;
    int charWidth;
    float spacing;
    float lineSpacing;
} data;

void DrawTringleTri(Triangle tri, Color color);

#endif // !SHARED
