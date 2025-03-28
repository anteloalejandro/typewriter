#include "raylib/include/raylib.h"
#include "raylib/include/raymath.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNPACK_RECT(rect) (rect).x, (rect).y, (rect).width, (rect).height
#define UNPACK_RECT_PAD(rect, n)                 \
    (rect).x - (n), (rect).y - (n),              \
    (rect).width + (n)*2, (rect).height + (n)*2
#define VECTOR2_RECT(rect) CLITERAL(Vector2) { (rect.x), (rect.y) }
#define VECTOR2_RECT_PAD(rect, n) CLITERAL(Vector2) { (rect.x) + (n), (rect.y) + (n) }

// TODO: 
// The cursor will be centered in a point, and cursorPos will be the closes available space in the Text.
// Typing will make the page itself move, until the end of the page it's reached

// STRUCTS
struct FontData {
    Font font;
    int fontSize;
    int charWidth;
    float spacing;
    float lineSpacing;
} fontData;

typedef struct {
    int x, y;
} Position;

typedef struct {
    int x, y;
    Vector2 a, b, c;
} Triangle;

typedef struct {
    char *str;
    int size;
    Vector2 position;
} Line;

// FUNCTIONS

void DrawTringleTri(Triangle tri, Color color) {
    DrawTriangle(
        Vector2Add(tri.a, (Vector2) { tri.x, tri.y }),
        Vector2Add(tri.b, (Vector2) { tri.x, tri.y }),
        Vector2Add(tri.c, (Vector2) { tri.x, tri.y }),
        color
    );
}

void Line_appendChar(Line *line, char c) {
    line->str = realloc(line->str, ++line->size);
    line->str[line->size-1] = c;
}

void Line_appendStr(Line *line, char *str, int length) {
    line->str = realloc(line->str, line->size + length);
    memcpy(&line->str[line->size], str, length);
    line->size += length;
}

void Line_destroy(Line *line) {
    free(line->str);
}

// works for either height and width
enum Orientation {VERTICAL, HORIZONTAL};
int textSize(int n, enum Orientation orientation) {
    int charSize, spacing;
    if (orientation == VERTICAL) {
        charSize = fontData.fontSize;
        spacing = fontData.lineSpacing;
    } else {
        charSize = fontData.charWidth;
        spacing = fontData.spacing;
    }
    if (n == 0) return 0;
    if (n == 1) return charSize + spacing;

    return n * (charSize + spacing);
}

bool fitsInRect(int nChars, int padding, Rectangle rect) {
    return textSize(nChars, HORIZONTAL) + fontData.charWidth + fontData.spacing <= rect.width-padding*2;
}

int MeasureChar(Font font, char c, int fontSize, int spacing) {
    char buf[2] = " "; buf[0] = c;
    return MeasureTextEx(font, buf, fontSize, spacing).x;
}

void setFont(char *filename, int fontSize) {
    fontData.fontSize = fontSize;
    fontData.font = LoadFontEx(filename, fontData.fontSize, 0, 250);
    // spacing = fontSize/defaultFontSize;
    fontData.spacing = 0;
    fontData.charWidth = MeasureChar(fontData.font, 'M', fontData.fontSize, fontData.spacing);
    fontData.lineSpacing = 0.5*fontData.fontSize; // 0.5 of height added to the linejump
}

int main()
{
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetTargetFPS(60);

    setFont("resources/UbuntuMono-R.ttf", 20);

    const int margin = 20;
    const int padding = 5;
    const int border = 3;
    const int rows = 20;
    const int cols = 80;

    Position cursorPos = (Position) {0, 0};

    Rectangle cursor = (Rectangle) {
        .width = fontData.charWidth,
        .height = fontData.fontSize,
        .x = (screenWidth - fontData.charWidth)/(float)2,
        .y = (screenHeight - fontData.fontSize)/(float)2
    };

    Rectangle container = (Rectangle) {
        .width =  textSize(cols, HORIZONTAL) + padding*2,
        .height = textSize(rows, VERTICAL),
        .y = cursor.y - padding,
        .x = cursor.x - padding 
    };

    Triangle carret = (Triangle) {
        .x = container.x- border - padding - fontData.fontSize/(float)2,
        .y = cursor.y,
        {0, 0}, {0, fontData.fontSize}, {fontData.charWidth, fontData.fontSize/(float)2}
    };

    Line lines[rows];
    for (int i = 0; i < rows; i++) {
        lines[i].str = NULL;
        lines[i].size = 0;
        lines[i].position.x = 0;
        lines[i].position.y = 0;

        char buf[cols+1];
        snprintf(buf, cols, "%*s", cols, "");
        Line_appendStr(lines+i, buf, cols);
    }

    while (!WindowShouldClose())
    {
        Line *line = lines + cursorPos.y;
        int key;
        while ((key = GetCharPressed()) > 0) {
            if ((key >= 33) && (key <= 125) && fitsInRect(cursorPos.x, padding, container)) {
                line->str[cursorPos.x++] = key;
            }
        }

        if (IsKeyPressed(KEY_SPACE) && fitsInRect(cursorPos.x, padding, container)) {
            cursorPos.x++;
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (IsKeyDown(KEY_LEFT_CONTROL)) {
                line->str[cursorPos.x] = ' ';
            } else if (cursorPos.x > 0) {
                cursorPos.x--;
            }
        }
        if (IsKeyPressed(KEY_HOME)) {
            cursorPos.x = 0;
        }
        if (IsKeyPressed(KEY_PAGE_UP)) {
            cursorPos.y = 0;
        }
        if (IsKeyPressed(KEY_ENTER) && cursorPos.y < rows-1) {
            cursorPos.y++;
        }

        container.x = (cursor.x - padding) - (textSize(cursorPos.x, HORIZONTAL));
        container.y = (cursor.y - padding) - (textSize(cursorPos.y, VERTICAL));
        carret.x = container.x- border - padding - fontData.fontSize/(float)2,
        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawRectangle(UNPACK_RECT_PAD(container, border), BLACK);
            DrawRectangleRec(container, WHITE);
            DrawRectangleRec(cursor, LIGHTGRAY);
            DrawTringleTri(carret, RED);
            for (int i = 0; i < rows; i++) {
                const Vector2 textPos = Vector2Add(VECTOR2_RECT_PAD(container, padding), (Vector2) {0, textSize(i, VERTICAL)});
                DrawTextEx(fontData.font, lines[i].str, textPos, fontData.fontSize, fontData.spacing, BLACK);
            }

            DrawText("CORRECTOR: CTRL+RETURN\nCR: HOME", 50, screenHeight-50, 20, BLACK);

            DrawLine(0, container.y, screenWidth, container.y, BLUE);
            DrawText("Y", container.x+margin, margin, 10, BLUE);
            DrawLine(container.x, 0, container.x, screenHeight, BLUE);
            DrawText("X", margin, container.y+margin, 10, BLUE);
        }
        EndDrawing();
    }

    for (int i = 0; i < rows; i++) Line_destroy(lines+i);
    UnloadFont(fontData.font);
    CloseWindow();
    return 0;
}
