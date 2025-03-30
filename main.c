#include "raylib/include/raylib.h"
#include "raylib/include/raymath.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// IMPORT RAYGUI IGNORING ERRORS IN THE HEADER FILE ITSELF
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#define RAYGUI_IMPLEMENTATION
#include "raylib/include/raygui.h"
#undef RAYGUI_IMPLEMENTATION
#pragma GCC diagnostic pop

#define UNPACK_RECT(rect) (rect).x, (rect).y, (rect).width, (rect).height
#define UNPACK_RECT_PAD(rect, n)                 \
    (rect).x - (n), (rect).y - (n),              \
    (rect).width + (n)*2, (rect).height + (n)*2
#define VECTOR2_RECT(rect) CLITERAL(Vector2) { (rect.x), (rect.y) }
#define VECTOR2_RECT_PAD(rect, n) CLITERAL(Vector2) { (rect.x) + (n), (rect.y) + (n) }
#define TRI_TO_RECT(tri) CLITERAL(Rectangle) {carret.x, carret.y, carret.c.x, carret.b.y}

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
} Line;

// STATIC VARIABLES

static struct {
    KeyboardKey keys[40];
    int frames[40];
    int initialFrames;
} keyboardKeys = {
    .keys = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO, // KEY_KP_SUBTRACT, KEY_KP_EQUAL,
        KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P,
        KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, // KEY_APOSTROPHE,
        KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH
    },
    .frames = { },
    .initialFrames = 5
};

// FUNCTIONS

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

void DrawTringleTri(Triangle tri, Color color) {
    DrawTriangle(
        Vector2Add(tri.a, (Vector2) { tri.x, tri.y }),
        Vector2Add(tri.b, (Vector2) { tri.x, tri.y }),
        Vector2Add(tri.c, (Vector2) { tri.x, tri.y }),
        color
    );
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

void keyResetFrames(KeyboardKey key) {
    for(int i = 0; i < (int) sizeof(keyboardKeys.keys); i++) {
        if (keyboardKeys.keys[i] == key) {
            keyboardKeys.frames[i] = keyboardKeys.initialFrames;
            return;
        }
    }
}

int GetForceLayoutKey() {
    // NOTE: CAPS_LOCK is broken on raylib and will always only be detected as DOWN once pressed,
    // and there is no way to change it. Thus, proper CAPS_LOCK behaviour cannot be implemented.
    int key = GetKeyPressed();
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        switch (key) {
            case KEY_SEMICOLON: key = ':'; break;
            case KEY_SLASH: key = '?'; break;
            case KEY_COMMA: key = '<'; break;
            case KEY_PERIOD: key = '>'; break;
            default: break;
        }
    } else {
        key = tolower(key);
    }
    return key;
}

int main()
{
    const int screenWidth = 1200;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetTargetFPS(60);

    bool showSettings = false;
    bool forceLayout = false;

    setFont("resources/UbuntuMono-R.ttf", 20);

    const int margin = 20;
    const int padding = 5;
    const int border = 3;
    const int rows = 20;
    const int cols = 80;
    const int scrollSpeed = 10;

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
        .x = 0, .y = 0,
        {0, 0}, {0, fontData.fontSize}, {fontData.charWidth, fontData.fontSize/(float)2}
    };

    float keyboardKeyRadius = 40;
    Rectangle keyboard = (Rectangle) {
        .height = (keyboardKeyRadius*2 + padding) * 4 + padding,
        .width = (keyboardKeyRadius*2 + padding) * 10 + padding,
    };
    keyboard.x = (screenWidth - keyboard.width)/2;
    keyboard.y = screenHeight - keyboard.height;

    Line lines[rows];
    for (int i = 0; i < rows; i++) {
        lines[i].str = NULL;
        lines[i].size = 0;

        char buf[cols+1];
        snprintf(buf, cols, "%*s", cols, "");
        Line_appendStr(lines+i, buf, cols);
    }

    while (!WindowShouldClose())
    {
        if (!showSettings) {
            Line *line = lines + cursorPos.y;
            int key = '\0';
            while ((key = (forceLayout ? GetForceLayoutKey() : GetCharPressed())) > 0) {
                if ((key >= 33) && (key <= 125) && fitsInRect(cursorPos.x, padding, container)) {
                    line->str[cursorPos.x++] = key;
                }
                keyResetFrames(toupper(key));
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
        }

        Vector2 moveDelta = Vector2Multiply(GetMouseWheelMoveV(), (Vector2) {scrollSpeed, scrollSpeed});
        if (cursor.x + moveDelta.x >= padding && cursor.x + moveDelta.x <= screenWidth - padding - fontData.charWidth)
            cursor.x += moveDelta.x;
        if (cursor.y + moveDelta.y >= padding && cursor.y + moveDelta.y <= screenHeight - padding - fontData.fontSize)
            cursor.y += moveDelta.y;

        static float mousePressedMove = 0;
        static bool carretPressed = false;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (carretPressed || CheckCollisionPointRec(GetMousePosition(), TRI_TO_RECT(carret)))) {
            carretPressed = true;
            mousePressedMove += GetMouseDelta().x; // multiply by charWidth to match mouse
            if (mousePressedMove > fontData.charWidth || -mousePressedMove > fontData.charWidth) {
                const int cursorMove = mousePressedMove / fontData.charWidth;
                mousePressedMove -= cursorMove * fontData.charWidth;
                int newCursorPos_x = cursorPos.x - cursorMove;
                if (newCursorPos_x <= 0) {
                    newCursorPos_x = 0;
                    if (cursorPos.y < rows-1 && newCursorPos_x != cursorPos.x)
                        cursorPos.y++;
                } else if (newCursorPos_x >= cols) {
                    newCursorPos_x = cols-1;
                }
                cursorPos.x = newCursorPos_x;
            }
        } else {
            carretPressed = false;
            mousePressedMove = 0;
        }

        container.x = (cursor.x - padding) - (textSize(cursorPos.x, HORIZONTAL));
        container.y = (cursor.y - padding) - (textSize(cursorPos.y, VERTICAL));
        carret.x = container.x - border - padding - fontData.fontSize/(float)2,
        carret.y = cursor.y;

        BeginDrawing();
        {
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            DrawRectangle(UNPACK_RECT_PAD(container, border), GetColor(GuiGetStyle(DEFAULT, showSettings ? BORDER_COLOR_DISABLED : BORDER_COLOR_NORMAL)));
            DrawRectangleRec(container, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            DrawRectangleRec(cursor, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));
            DrawTringleTri(carret, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_PRESSED)));
            for (int i = 0; i < rows; i++) {
                const Vector2 textPos = Vector2Add(VECTOR2_RECT_PAD(container, padding), (Vector2) {0, textSize(i, VERTICAL)});
                DrawTextEx(fontData.font, lines[i].str, textPos, fontData.fontSize, fontData.spacing, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
            }

            DrawRectangle(0, keyboard.y, screenWidth, keyboard.height, GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_DISABLED)));
            DrawRectangleRec(keyboard, GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_DISABLED)));
            for (int i = 0; i < 40; i++) {

                int column = i%10;
                int row = i/10;

                Vector2 center = (Vector2) {
                    keyboard.x + padding + keyboardKeyRadius + column * (keyboardKeyRadius*2 + padding),
                    keyboard.y + padding + keyboardKeyRadius + row * (keyboardKeyRadius*2 + padding)
                };
                
                char buf[] = {(char) keyboardKeys.keys[i], '\0'};
                const int keyFontSize = keyboardKeyRadius*0.8;
                const int keyCharWidth = MeasureText(buf, keyFontSize);

                DrawCircleV(center, keyboardKeyRadius, keyboardKeys.frames[i] ? GetColor(GuiGetStyle(DEFAULT, BASE_COLOR_DISABLED)) : GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
                DrawText(buf, center.x - keyCharWidth/(float)2, center.y - keyFontSize/(float)2, keyFontSize, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));


                keyboardKeys.frames[i] -= keyboardKeys.frames[i] == 0 ? 0 : 1;
            }

            if (showSettings) {
                DrawRectangle(screenWidth-220, 0, 220, screenHeight, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_DISABLED)));
                GuiToggle((Rectangle) {screenWidth-200, 60, 180, 30}, forceLayout ? "#89# Force Typewriter Layout" : "#80# Force Typewriter Layout", &forceLayout);
            }

            GuiToggle((Rectangle){screenWidth-padding-30, padding, 30, 30}, "#214#", &showSettings);

            DrawLine(0, container.y, screenWidth, container.y, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            DrawText("X", container.x+margin, margin, 10, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            DrawLine(container.x, 0, container.x, screenHeight, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
            DrawText("Y", margin, container.y+margin, 10, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)));
        }
        EndDrawing();
    }

    for (int i = 0; i < rows; i++) Line_destroy(lines+i);
    UnloadFont(fontData.font);
    CloseWindow();
    return 0;
}
