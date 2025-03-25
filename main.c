#include "raylib/include/raylib.h"
#include "headers/word-wrap.h"

#define UNPACK_RECT(rect) (rect).x, (rect).y, (rect).width, (rect).height
#define UNPACK_RECT_PAD(rect, n)                 \
    (rect).x - (n), (rect).y - (n),              \
    (rect).width + (n)*2, (rect).height + (n)*2  \

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 400;

    const char text[] =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magnam aliquam quaerat voluptatem. Itaque earum rerum defuturum, quas natura non depravata desiderat. Et quem ad me accedis, saluto: 'chaere,' inquam, 'Tite!' lictores, turma omnis chorusque: 'chaere, Tite!' hinc hostis mi Albucius, hinc inimicus. Sed iure Mucius.";

    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetTargetFPS(60);

    const int containerMargin = 20;
    Rectangle container = (Rectangle) {
        .width = 500, .height = 300,
        .x = (screenWidth-500-containerMargin*2)/(float)2 + containerMargin, .y = containerMargin,
    };

    while (!WindowShouldClose())
    {
        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawRectangle(UNPACK_RECT_PAD(container, 5), BLACK);
            DrawRectangleRec(container, WHITE);
            // spacing = fontSize/defaultFontSize;
            DrawTextBoxed(GetFontDefault(), text, (Rectangle) { UNPACK_RECT_PAD(container, -5) }, 20, 20/GetFontDefault().baseSize, true, BLACK);
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
