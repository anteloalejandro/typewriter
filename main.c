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

typedef struct {
    int x, y;
} Position;

typedef struct {
    char *str;
    int size;
} Line;

void Line_appendChar(Line *line, char c) {
    line->str = realloc(line->str, ++line->size);
    line->str[line->size-1] = c;
}

void Line_appendStr(Line *line, char *str, int length) {
    line->str = realloc(line->str, line->size + length);
    memcpy(&line->str[line->size], str, length);
    line->size += length;
}

// works for either height and width
int textSize(int n, int charSize, float spacing) {
    if (n == 0) return 0;
    if (n == 1) return charSize + spacing;

    return n * (charSize + spacing);
}

bool fitsInRect(int nChars, int charWidth, int spacing, int padding, Rectangle rect) {
    return textSize(nChars, charWidth, spacing) + charWidth + spacing < rect.width-padding*2;
}

void Line_destroy(Line *line) {
    free(line->str);
}

typedef struct {
    Line *lines;
    int rows, cols;
} Text;

void Text_appendLine(Text *text, Line *line) {
    text->lines = realloc(text->lines, sizeof(Line) * ++text->rows);
    text->lines[text->rows-1] = *line;
}

void Text_appendEmptyLine(Text *text) {
    Line line = {NULL, 0};
    char buf[text->cols];
    snprintf(buf, text->cols, "%*s", text->cols, "");
    Line_appendStr(&line, buf, text->cols);
    Text_appendLine(text, &line);
}

void Text_destroy(Text *text) {
    for (int i = 0; i < text->rows; i++) {
        Line_destroy(&text->lines[i]);
        printf("a");
    }
    free(text->lines);
}

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
    const float lineSpacing = 0.5*fontSize; // 0.5 of height added to the linejump

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

    Text text = (Text) {
        NULL, .rows = 0, .cols = (container.width-padding*2)/charWidth
    };
    Text_appendEmptyLine(&text);

    while (!WindowShouldClose())
    {
        Line *line = text.lines + cursorPos.y;
        int key;
        while ((key = GetCharPressed()) > 0) {
            if ((key >= 33) && (key <= 125) && fitsInRect(cursorPos.x, charWidth, spacing, padding, container)) {
                line->str[cursorPos.x++] = key;
            }
        }

        if (IsKeyPressed(KEY_SPACE) && fitsInRect(cursorPos.x, charWidth, spacing, padding, container)) {
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
        if (IsKeyPressed(KEY_ENTER)) {
            cursorPos.y++;
            if (cursorPos.y >= text.rows) {
                Text_appendEmptyLine(&text);
            }
        }

        cursor.x = container.x + padding + textSize(cursorPos.x, charWidth, spacing);
        cursor.y = container.y + padding + textSize(cursorPos.y, fontSize, lineSpacing);
        container.height = textSize(text.rows, fontSize, lineSpacing) + padding*2 - lineSpacing;

        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawRectangle(UNPACK_RECT_PAD(container, border), BLACK);
            DrawRectangleRec(container, WHITE);
            DrawRectangleRec(cursor, LIGHTGRAY);
            for (int i = 0; i < text.rows; i++) {
                const Vector2 textPos = Vector2Add(VECTOR2_RECT_PAD(container, padding), (Vector2) {0, textSize(i, fontSize, lineSpacing)});
                DrawTextEx(font, text.lines[i].str, textPos, font.baseSize, spacing, BLACK);
            }

            DrawText("CORRECTOR: CTRL+RETURN\nCR: HOME", container.x, screenHeight-50, 20, BLACK);
        }
        EndDrawing();
    }

    Text_destroy(&text);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
