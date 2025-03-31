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
#define GUI_COLOR(color) GetColor(GuiGetStyle(DEFAULT, color))
#define GUI_ICON(icon) "#" #icon "#"
#define TRANSPARENTIZE(color, alpha) CLITERAL(Color) {color.r, color.g, color.b, alpha}

// STRUCTS
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
    Font font;
    int fontSize;
    int charWidth;
    float spacing;
    float lineSpacing;
} data;


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
    .initialFrames = 10
};

// FUNCTIONS

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
        charSize = data.fontSize;
        spacing = data.lineSpacing;
    } else {
        charSize = data.charWidth;
        spacing = data.spacing;
    }
    if (n == 0) return 0;
    if (n == 1) return charSize + spacing;

    return n * (charSize + spacing);
}

bool fitsInRect(int nChars, int padding, Rectangle rect) {
    return textSize(nChars, HORIZONTAL) + data.charWidth + data.spacing <= rect.width-padding*2;
}

int MeasureChar(Font font, char c, int fontSize, int spacing) {
    char buf[2] = " "; buf[0] = c;
    return MeasureTextEx(font, buf, fontSize, spacing).x;
}

void setFont(char *filename, int fontSize) {
    data.fontSize = fontSize;
    data.font = LoadFontEx(filename, data.fontSize, 0, 250);
    // spacing = fontSize/defaultFontSize;
    data.spacing = 0;
    data.charWidth = MeasureChar(data.font, 'M', data.fontSize, data.spacing);
    data.lineSpacing = 0.5*data.fontSize; // 0.5 of height added to the linejump
}

void keyResetFrames(KeyboardKey key) {
    for(int i = 0; i < (int) sizeof(keyboardKeys.keys); i++) {
        if (keyboardKeys.keys[i] == key) {
            keyboardKeys.frames[i] = keyboardKeys.initialFrames;
            return;
        }
    }
}

int getForceLayoutKey() {
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
    bool hideKeyboard = false;

    setFont("resources/UbuntuMono-R.ttf", 20);

    const int margin = 20;
    const int padding = 5;
    const int border = 3;
    const int rows = 20;
    const int cols = 80;
    const int scrollSpeed = 10;

    Position cursorPos = (Position) {0, 0};

    Rectangle cursor = (Rectangle) {
        .width = data.charWidth,
        .height = data.fontSize,
        .x = (screenWidth - data.charWidth)/(float)2,
        .y = (screenHeight - data.fontSize)/(float)2
    };

    Rectangle container = (Rectangle) {
        .width =  textSize(cols, HORIZONTAL) + padding*2,
        .height = textSize(rows, VERTICAL),
        .y = cursor.y - padding,
        .x = cursor.x - padding 
    };

    Triangle carret = (Triangle) {
        .x = 0, .y = 0,
        {0, 0}, {0, data.fontSize}, {data.charWidth, data.fontSize/(float)2}
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
            while ((key = (forceLayout ? getForceLayoutKey() : GetCharPressed())) > 0) {
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
        if (cursor.x + moveDelta.x >= padding && cursor.x + moveDelta.x <= screenWidth - padding - data.charWidth)
            cursor.x += moveDelta.x;
        if (cursor.y + moveDelta.y >= padding && cursor.y + moveDelta.y <= screenHeight - padding - data.fontSize)
            cursor.y += moveDelta.y;

        static float mousePressedMove = 0;
        static bool carretPressed = false;
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (carretPressed || CheckCollisionPointRec(GetMousePosition(), TRI_TO_RECT(carret)))) {
            carretPressed = true;
            mousePressedMove += GetMouseDelta().x; // multiply by charWidth to match mouse
            if (mousePressedMove > data.charWidth || -mousePressedMove > data.charWidth) {
                const int cursorMove = mousePressedMove / data.charWidth;
                mousePressedMove -= cursorMove * data.charWidth;
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
        carret.x = container.x - border - padding - data.fontSize/(float)2,
        carret.y = cursor.y;

        BeginDrawing();
        {
            ClearBackground(GUI_COLOR(BACKGROUND_COLOR));
            DrawRectangle(UNPACK_RECT_PAD(container, border), GUI_COLOR(showSettings ? BORDER_COLOR_DISABLED : BORDER_COLOR_NORMAL));
            DrawRectangleRec(container, GUI_COLOR(BACKGROUND_COLOR));
            DrawRectangleRec(cursor, GUI_COLOR(TEXT_COLOR_DISABLED));
            DrawTringleTri(carret, GUI_COLOR(BORDER_COLOR_PRESSED));
            for (int i = 0; i < rows; i++) {
                const Vector2 textPos = Vector2Add(VECTOR2_RECT_PAD(container, padding), (Vector2) {0, textSize(i, VERTICAL)});
                DrawTextEx(data.font, lines[i].str, textPos, data.fontSize, data.spacing, GUI_COLOR(TEXT_COLOR_NORMAL));
            }

            if (!hideKeyboard) {
                DrawRectangle(0, keyboard.y, screenWidth, keyboard.height, GUI_COLOR(BASE_COLOR_DISABLED));
                DrawRectangleRec(keyboard, GUI_COLOR(BASE_COLOR_DISABLED));
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

                    if (keyboardKeys.frames[i]/(float)keyboardKeys.initialFrames > 0) printf("%.2f\n", Lerp(0, 255, keyboardKeys.frames[i]/(float)keyboardKeys.initialFrames));
                    DrawCircleV(center, keyboardKeyRadius, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - Lerp(0, 255, keyboardKeys.frames[i]/(float)keyboardKeys.initialFrames)));
                    DrawText(buf, center.x - keyCharWidth/(float)2, center.y - keyFontSize/(float)2, keyFontSize, GUI_COLOR(TEXT_COLOR_NORMAL));


                    keyboardKeys.frames[i] -= keyboardKeys.frames[i] == 0 ? 0 : 1;
                }
            }

            if (showSettings) {
                DrawRectangle(screenWidth-220, 0, 220, screenHeight, GUI_COLOR(BORDER_COLOR_DISABLED));
                GuiToggle((Rectangle) {screenWidth-200, 60, 180, 30}, forceLayout ? GUI_ICON(89) "Force Typewriter Layout" : GUI_ICON(80) "Force Typewriter Layout", &forceLayout);
                GuiToggle((Rectangle) {screenWidth-200, 100, 180, 30}, hideKeyboard ? GUI_ICON(89) "Hide Keyboard" : GUI_ICON(80) "Hide Keyboard", &hideKeyboard);
            }

            GuiToggle((Rectangle){screenWidth-padding-30, padding, 30, 30}, GUI_ICON(214), &showSettings);

            DrawLine(0, container.y, screenWidth, container.y, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawText("X", container.x+margin, margin, 10, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawLine(container.x, 0, container.x, screenHeight, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawText("Y", margin, container.y+margin, 10, GUI_COLOR(BORDER_COLOR_FOCUSED));
        }
        EndDrawing();
    }

    for (int i = 0; i < rows; i++) Line_destroy(lines+i);
    UnloadFont(data.font);
    CloseWindow();
    return 0;
}
