#include "raylib/include/raylib.h"
#include <stdio.h>
#include <ctype.h>

#define UNPACK_RECT(rect) (rect).x, (rect).y, (rect).width, (rect).height
#define UNPACK_RECT_PAD(rect, n)                 \
    (rect).x - (n), (rect).y - (n),              \
    (rect).width + (n)*2, (rect).height + (n)*2
#define POSITION_TO_VECTOR2(pos) CLITERAL(Vector2) {(pos).x, (pos).y}
#define VECTOR2_TO_POSITION(vec) CLITERAL(position) {(int)(vec).x, (int)(vec).y}

// TODO: Looks like text must be monospaced for the effect to work correctly

typedef struct {
    int x, y;
} Position;

int MeasureChar(char c, int fontSize) {
    char buf[2] = " "; buf[0] = c;
    return MeasureText(buf, fontSize);
}

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 400;
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetTargetFPS(60);

    const int fontSize = 20; // if using the default font, should be a multiple of 10
    // spacing = fontSize/defaultFontSize;
    const int spacing = fontSize/GetFontDefault().baseSize;
    char text[512];
    for (int i = 0; i < sizeof(text)-2; i++) {
        text[i] = ' ';
    }
    text[sizeof(text)-1] = '\0';
    int textLength = 0;

    const int margin = 20;
    const int padding = 5;
    const int border = 5;
    Rectangle container = (Rectangle) {
       .width = 500, .height = fontSize+padding*2, .y = margin
    };
    container.x = (screenWidth-container.width-margin*2)/(float)2 + margin;

    Position cursorPos = (Position) {0, 0};
    Rectangle cursor = (Rectangle) {
        .width = MeasureChar('M', fontSize),
        .height = fontSize,
        .x = container.x + padding,
        .y = container.y + padding
    };

    int lineWidth = 0;

    while (!WindowShouldClose())
    {
        int key;
        int charWidth;
        while ((key = GetCharPressed()) > 0) {
            charWidth = MeasureChar(toupper(key), fontSize);
            if ((key >= 33) && (key <= 125) && (lineWidth + charWidth + spacing < container.width-padding*2)) {
                text[cursorPos.x++] = toupper(key);
                textLength = cursorPos.x > textLength ? cursorPos.x : textLength;
                lineWidth += charWidth + spacing;
                printf("%d\n", key);
            }
        }

        if (IsKeyPressed(KEY_SPACE) && (lineWidth + cursor.width + spacing < container.width-padding*2)) {
            lineWidth += MeasureChar(text[cursorPos.x++], fontSize) + spacing;
            textLength = cursorPos.x > textLength ? cursorPos.x : textLength;
        }
        if (IsKeyPressed(KEY_BACKSPACE) && cursorPos.x > 0) {
            cursorPos.x--;
            lineWidth -= MeasureChar(text[cursorPos.x], fontSize) + spacing;
        }
        cursor.x = container.x + padding + lineWidth;

        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawRectangle(UNPACK_RECT_PAD(container, border), BLACK);
            DrawRectangleRec(container, WHITE);
            DrawRectangleRec(cursor, LIGHTGRAY);
            DrawText(text, container.x+padding, container.y+padding, fontSize, BLACK);       // Draw text (using default font)
            /* DrawTextBoxed(GetFontDefault(), text, (Rectangle) { UNPACK_RECT_PAD(container, -padding) }, fontSize, spacing, true, BLACK); */
        }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
