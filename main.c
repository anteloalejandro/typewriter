#include "raylib/include/raylib.h"
#include "raylib/include/raymath.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

// TODO: Use a better data structure that allows deletion, like a linked list
typedef struct {
    char c;
    int permanence;
    int visibility;
    Position position;
} FloatingChar;
typedef struct FloatingCharNode {
    FloatingChar *fchar;
    struct FloatingCharNode *next;
} FloatingCharNode;
typedef struct {
    FloatingCharNode *head;
    int length;
} FloatingCharList;

// STATIC VARIABLES
static struct {
    Font font;
    int fontSize;
    int charWidth;
    float spacing;
    float lineSpacing;
} data;

#define KEYBOARD_KEYS \
X(0, KEY_ONE) X(1, KEY_TWO) X(2, KEY_THREE) X(3, KEY_FOUR) X(4, KEY_FIVE) X(5, KEY_SIX) X(6, KEY_SEVEN) X(7, KEY_EIGHT) X(8, KEY_NINE) X(9, KEY_ZERO) \
X(10, KEY_Q) X(11, KEY_W) X(12, KEY_E) X(13, KEY_R) X(14, KEY_T) X(15, KEY_Y) X(16, KEY_U) X(17, KEY_I) X(18, KEY_O) X(19, KEY_P) \
X(20, KEY_A) X(21, KEY_S) X(22, KEY_D) X(23, KEY_F) X(24, KEY_G) X(25, KEY_H) X(26, KEY_J) X(27, KEY_K) X(28, KEY_L) \
X(29, KEY_Z) X(30, KEY_X) X(31, KEY_C) X(32, KEY_V) X(33, KEY_B) X(34, KEY_N) X(35, KEY_M)

static struct {
    KeyboardKey keys[36];
    int frames[36];
    int rowLength[4];
    int initialFrames;
} keyboardKeys = {
    .rowLength = { 10, 10, 9, 7 },
    .keys = {
        #define X(i, key) key,
        KEYBOARD_KEYS
        #undef X
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

void FloatingChar_destroy(FloatingCharNode *node) {
    free(node->fchar);
    free(node);
}

void FloatingCharList_insert(FloatingCharList *list, char c, Position position) {
    FloatingChar *fchar = malloc(sizeof(FloatingChar));
    FloatingCharNode *node = malloc(sizeof(FloatingCharNode));

    fchar->c = c;
    fchar->position = position;
    fchar->permanence = GetRandomValue(2, 5);
    fchar->visibility = fchar->permanence;

    node->fchar = fchar;
    node->next = list->head;
    list->head = node;

    list->length++;

}

void printList(FloatingCharList *list) {
    FloatingCharNode *node = list->head;
    while (node != NULL) {
        FloatingChar m = *node->fchar;
        printf("{ %c, %d, %d, (%d,%d) } -> ", m.c, m.permanence, m.visibility, m.position.x, m.position.y);
        node = node->next;
    }
    printf("NULL\n");
}

void FloatingCharList_eraseAll(FloatingCharList *list, Position position) {
    if (list->head == NULL) return;
    FloatingCharNode *prev = NULL, *node = list->head;
    while (node != NULL) {
        FloatingChar *m = node->fchar;
        if (m->position.x == position.x && m->position.y == position.y) {
            if (m->visibility <= 1) {
                FloatingCharNode *tmp = node;
                node = node->next;
                if (prev == NULL) list->head = node;
                else prev->next = node;
                FloatingChar_destroy(tmp);
            } else {
                m->visibility--;
                prev = node;
                node = node->next;
            }
        } else {
            prev = node;
            node = node->next;
        }
    }
    printList(list);
}

void FloatingCharList_destroy(FloatingCharList *list) {
    FloatingCharNode *node = list->head;
    while (node != NULL) {
        FloatingCharNode *tmp = node;
        node = node->next;
        FloatingChar_destroy(tmp);
    }
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

int getKeysIndex(KeyboardKey key) {
    switch (key) {
        #define X(i, name) case name: return i;
        KEYBOARD_KEYS
        #undef X
        default: return -1;
    }
}

void keyResetFrames(KeyboardKey key) {
    int i = getKeysIndex(key);
    if (i == -1) return;
    keyboardKeys.frames[i] = keyboardKeys.initialFrames;
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
    int screenWidth = 1200;
    int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    SetRandomSeed(time(NULL));

    bool showSettings = false;
    bool forceLayout = false;
    bool hideKeyboard = false;
    bool showFloatingChars = false;

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

    float keyboardKeyRadius = 30;
    Rectangle keyboard = (Rectangle) {
        .height = (keyboardKeyRadius*2 + padding) * 4 + padding,
        .width = (keyboardKeyRadius*2 + padding) * 10 + padding,
    };

    Line lines[rows];
    for (int i = 0; i < rows; i++) {
        lines[i].str = NULL;
        lines[i].size = 0;

        char buf[cols+1];
        snprintf(buf, cols, "%*s", cols, "");
        Line_appendStr(lines+i, buf, cols);
    }

    FloatingCharList fchars = { NULL, 0 };

    while (!WindowShouldClose())
    {
        static int prevScreenWidth = 0;
        static int prevScreenHeight = 0;
        prevScreenWidth = screenWidth;
        prevScreenHeight = screenHeight;
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        if (!showSettings) {
            Line *line = lines + cursorPos.y;
            int key = '\0';
            while ((key = (forceLayout ? getForceLayoutKey() : GetCharPressed())) > 0) {
                if ((key >= 33) && (key <= 125) && fitsInRect(cursorPos.x, padding, container)) {
                    if (line->str[cursorPos.x] != ' ') {
                        FloatingCharList_insert(&fchars, line->str[cursorPos.x], cursorPos);
                    }
                    line->str[cursorPos.x++] = key;
                }
                keyResetFrames(toupper(key));
            }


            if (IsKeyPressed(KEY_SPACE) && fitsInRect(cursorPos.x, padding, container)) {
                cursorPos.x++;
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                if (IsKeyDown(KEY_LEFT_CONTROL)) {
                    if (line->str[cursorPos.x] != ' ') {
                        FloatingCharList_insert(&fchars, line->str[cursorPos.x], cursorPos);
                        line->str[cursorPos.x] = ' ';
                    }
                    FloatingCharList_eraseAll(&fchars, cursorPos);
                } else if (cursorPos.x > 0) {
                    cursorPos.x--;
                }
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

        if (prevScreenWidth != screenWidth || prevScreenHeight != screenHeight) {
            cursor.x += (screenWidth - prevScreenWidth)/2.0;
            cursor.y += (screenHeight - prevScreenHeight)/2.0;
        }

        container.x = (cursor.x - padding) - (textSize(cursorPos.x, HORIZONTAL));
        container.y = (cursor.y - padding) - (textSize(cursorPos.y, VERTICAL));
        carret.x = container.x - border - padding - data.fontSize/(float)2,
        carret.y = cursor.y;

        keyboard.x = (screenWidth - keyboard.width)/2;
        keyboard.y = screenHeight - keyboard.height;

        BeginDrawing();
        {
            ClearBackground(GUI_COLOR(BACKGROUND_COLOR));
            DrawRectangle(UNPACK_RECT_PAD(container, border), GUI_COLOR(showSettings ? BORDER_COLOR_DISABLED : BORDER_COLOR_NORMAL));
            DrawRectangleRec(container, GUI_COLOR(BACKGROUND_COLOR));
            DrawRectangleRec(cursor, GUI_COLOR(TEXT_COLOR_DISABLED));
            DrawTringleTri(carret, GUI_COLOR(BORDER_COLOR_PRESSED));
            if (showFloatingChars) {
                FloatingCharNode *node = fchars.head;
                while (node != NULL) {
                    char buf[2] = {node->fchar->c, '\0'};
                    Vector2 pos = {
                        textSize(node->fchar->position.x, HORIZONTAL) + container.x + padding,
                        textSize(node->fchar->position.y, VERTICAL) + container.y + padding,
                    };

                    const unsigned char transparency = Lerp(0, 255, node->fchar->visibility/(float)node->fchar->permanence);
                    Color color = TRANSPARENTIZE(GUI_COLOR(TEXT_COLOR_NORMAL), transparency);
                    DrawTextEx(data.font, buf, pos, data.fontSize, data.spacing, color);
                    node = node->next;
                }
            }
            for (int i = 0; i < rows; i++) {
                const Vector2 textPos = Vector2Add(VECTOR2_RECT_PAD(container, padding), (Vector2) {0, textSize(i, VERTICAL)});
                DrawTextEx(data.font, lines[i].str, textPos, data.fontSize, data.spacing, GUI_COLOR(TEXT_COLOR_NORMAL));
            }

            {
                static int frames = 0;
                const int keyboardFrames = 5;
                if (hideKeyboard) frames++;
                else frames--;
                frames = Clamp(frames, 0, keyboardFrames);
                keyboard.y = Lerp(screenHeight-keyboard.height, screenHeight, frames/(float)keyboardFrames);
                if (!hideKeyboard || frames < keyboardFrames) {
                    DrawRectangle(0, keyboard.y, screenWidth, keyboard.height, GUI_COLOR(BASE_COLOR_DISABLED));
                    DrawRectangleRec(keyboard, GUI_COLOR(BASE_COLOR_DISABLED));
                    for (int row = 0, i = 0; row < 4; row++) {
                        const float columnOffset = (keyboardKeys.rowLength[0]-keyboardKeys.rowLength[row])/2.0;
                        for (int column = 0; column < keyboardKeys.rowLength[row]; column++, i++) {

                            Vector2 center = (Vector2) {
                                keyboard.x + padding + keyboardKeyRadius + (column+columnOffset) * (keyboardKeyRadius*2 + padding),
                                keyboard.y + padding + keyboardKeyRadius + row * (keyboardKeyRadius*2 + padding)
                            };

                            char buf[] = {(char) keyboardKeys.keys[i], '\0'};
                            const int keyFontSize = keyboardKeyRadius*0.67;
                            const int keyCharWidth = MeasureText(buf, keyFontSize);

                            const unsigned char transparency = Lerp(0, 255, keyboardKeys.frames[i]/(float)keyboardKeys.initialFrames);
                            DrawCircleV(center, keyboardKeyRadius, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
                            DrawText(buf, center.x - keyCharWidth/(float)2, center.y - keyFontSize/(float)2, keyFontSize, GUI_COLOR(TEXT_COLOR_NORMAL));

                            keyboardKeys.frames[i] -= keyboardKeys.frames[i] == 0 ? 0 : 1;
                        }
                    }
                }
            }

            {
                const int settingsWidth = 220;
                const int finalSettingsX = screenWidth-settingsWidth;
                const int settingsFrames = 10;
                static int frames = 0;
                int settingsX = 0;
                if (showSettings) frames++;
                else frames--;
                frames = Clamp(frames, 0, settingsFrames);
                settingsX = Lerp(screenWidth, finalSettingsX, frames/(float)settingsFrames);
                DrawRectangle(settingsX, 0, settingsWidth, screenHeight, GUI_COLOR(BORDER_COLOR_DISABLED));
                GuiToggle(
                    (Rectangle) {settingsX+padding, 60, settingsWidth-padding*2, 30},
                    forceLayout ? GUI_ICON(89) "Force Typewriter Layout" : GUI_ICON(80) "Force Typewriter Layout",
                    &forceLayout
                );
                GuiToggle(
                    (Rectangle) {settingsX+padding, 100, settingsWidth-padding*2, 30},
                    hideKeyboard ? GUI_ICON(89) "Hide Keyboard" : GUI_ICON(80) "Hide Keyboard",
                    &hideKeyboard
                );
                GuiToggle(
                    (Rectangle) {settingsX+padding, 140, settingsWidth-padding*2, 30},
                    showFloatingChars ? GUI_ICON(89) "Enable character overlapping" : GUI_ICON(80) "Enable character overlapping",
                    &showFloatingChars
                );
                if (GuiButton((Rectangle) {settingsX+padding, 180, settingsWidth-padding*2, 30}, "Clear Floating Chars")) {
                    FloatingCharList_destroy(&fchars);
                    fchars.head = NULL; fchars.length = 0;
                }
            }

            GuiToggle((Rectangle){screenWidth-padding-30, padding, 30, 30}, showSettings ? GUI_ICON(113) : GUI_ICON(214), &showSettings);

            DrawLine(0, container.y, screenWidth, container.y, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawText("X", container.x+margin, margin, 10, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawLine(container.x, 0, container.x, screenHeight, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawText("Y", margin, container.y+margin, 10, GUI_COLOR(BORDER_COLOR_FOCUSED));
        }
        EndDrawing();
    }

    for (int i = 0; i < rows; i++) Line_destroy(lines+i);
    FloatingCharList_destroy(&fchars);
    UnloadFont(data.font);
    CloseWindow();
    return 0;
}
