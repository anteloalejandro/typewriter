#include "raylib/include/raylib.h"
#include <stdio.h>

#define UNPACK_RECT(rect) (rect).x, (rect).y, (rect).width, (rect).height
#define UNPACK_RECT_PAD(rect, n)                 \
    (rect).x - (n), (rect).y - (n),              \
    (rect).width + (n)*2, (rect).height + (n)*2
#define VECTOR2_RECT(rect) CLITERAL(Vector2) { (rect.x), (rect.y) }
#define VECTOR2_RECT_PAD(rect, n) CLITERAL(Vector2) { (rect.x) + (n), (rect.y) + (n) }

// TODO: Looks like text must be monospaced for the effect to work correctly

typedef struct {
    int x, y;
} Position;

int MeasureChar(Font font, char c, int fontSize, int spacing) {
    char buf[2] = " "; buf[0] = c;
    return MeasureTextEx(font, buf, fontSize, spacing).x;
}

int main()
{
    const int screenWidth = 800;
    const int screenHeight = 400;
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetTargetFPS(60);

    const int fontSize = 20; // if using the default font, should be a multiple of 10
    Font font = LoadFontEx("resources/UbuntuMono-R.ttf", fontSize, 0, 250);
    // spacing = fontSize/defaultFontSize;
    const int spacing = 0;
    const int charWidth = MeasureChar(font, 'M', fontSize, spacing);

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
        .width = charWidth,
        .height = fontSize,
        .x = container.x + padding,
        .y = container.y + padding
    };

    int lineWidth = 0;

    while (!WindowShouldClose())
    {
        int key;
        while ((key = GetCharPressed()) > 0) {
            if ((key >= 33) && (key <= 125) && (lineWidth + charWidth + spacing < container.width-padding*2)) {
                text[cursorPos.x++] = key;
                textLength = cursorPos.x > textLength ? cursorPos.x : textLength;
                lineWidth += charWidth + spacing;
                printf("%d\n", key);
            }
        }

        if (IsKeyPressed(KEY_SPACE) && (lineWidth + cursor.width + spacing < container.width-padding*2)) {
            lineWidth += charWidth + spacing;
            cursorPos.x++;
            textLength = cursorPos.x > textLength ? cursorPos.x : textLength;
        }
        if (IsKeyPressed(KEY_BACKSPACE) && cursorPos.x > 0) {
            cursorPos.x--;
            lineWidth -= charWidth + spacing;
        }
        cursor.x = container.x + padding + lineWidth;

        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawRectangle(UNPACK_RECT_PAD(container, border), BLACK);
            DrawRectangleRec(container, WHITE);
            DrawRectangleRec(cursor, LIGHTGRAY);
            DrawTextEx(font, text, VECTOR2_RECT_PAD(container, padding), font.baseSize, spacing, BLACK);
            /* DrawTextBoxed(GetFontDefault(), text, (Rectangle) { UNPACK_RECT_PAD(container, -padding) }, fontSize, spacing, true, BLACK); */
        }
        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
