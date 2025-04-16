#include "raylib/include/raylib.h"
#include "raylib/include/raymath.h"
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "headers/shared.h"
#include "src/floatingchars.c"
#include "src/text_handling.c"
#include "src/input.c"

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
#define UNPACK_VECTOR2(v) (v).x, (v).y
#define VECTOR2(shape) CLITERAL(Vector2) { (shape.x), (shape.y) }
#define TRI_TO_RECT(tri) CLITERAL(Rectangle) {carret.x, carret.y, carret.c.x, carret.b.y}
#define GUI_COLOR(color) GetColor(GuiGetStyle(DEFAULT, color))
#define GUI_TOGGLE_TEXT(isTrue, str) GuiIconText((isTrue) ? ICON_BOX_CENTER : ICON_BOX, str)
#define TRANSPARENTIZE(color, alpha) CLITERAL(Color) {color.r, color.g, color.b, alpha}

struct FontItem {
    char *path;
    int fontSize;
} fonts[] = {
    { "resources/UbuntuMono-R.ttf", 20 },
    { "resources/cour.ttf", 20 },
};
struct ThemeItem {
    char *path;
} themes[] = {
    { NULL },
    { "resources/style_dark.rgs" }
};

int screenWidth = 1200;
int screenHeight = 800;

bool showSettings = false;
bool emulateLayout = true;
bool hideKeyboard = false;
bool enableOverlapping = true;

const int margin = 20;
const int padding = 5;
const int border = 3;
const int rows = 20;
const int cols = 80;
const int scrollSpeed = 15;
const float keyRadius = 30;

Position cursorPos;
Rectangle cursor;
Rectangle container;
Triangle carret;
Rectangle keyboard;
Str *lines = NULL;
FloatingCharList fchars = { NULL, 0 };

// reloads the font after GuiLoadStyle breaks it.
void setTheme(char *theme, char *font, int fontSize) {
    if (theme == NULL) {
        GuiLoadStyleDefault();
    } else {
        GuiLoadStyle(theme);
    }
    setFont(font, fontSize);
}

void init() {
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    SetExitKey(0); // Disable exit key

    SetRandomSeed(time(NULL));

    setTheme(themes[0].path, fonts[0].path, fonts[0].fontSize);

    cursorPos = (Position) {0, 0};
    fchars = (FloatingCharList) { NULL, 0 };

    cursor = (Rectangle) {
        .width = data.charWidth,
        .height = data.fontSize,
        .x = (screenWidth - data.charWidth)/(float)2,
        .y = (screenHeight - data.fontSize)/(float)2
    };

    container = (Rectangle) {
        .width =  textSize(cols, HORIZONTAL) + padding*2,
        .height = textSize(rows, VERTICAL),
        .y = cursor.y - padding,
        .x = cursor.x - padding 
    };

    carret = (Triangle) {
        .x = 0, .y = 0,
        {0, 0}, {0, data.fontSize}, {data.charWidth, data.fontSize/(float)2}
    };

    keyboard = (Rectangle) {
        .width = screenWidth,
        .y = INFINITY
    };

    lines = realloc(lines, sizeof(Str)*rows);
    for (int i = 0; i < rows; i++) {
        STR_INIT(lines[i]);

        char buf[cols+1];
        snprintf(buf, cols, "%*s", cols, "");
        Str_append(lines+i, buf, cols);
    }
}

void closeAndFree() {
    for (int i = 0; i < rows; i++) Str_destroy(lines+i);
    free(lines);
    FloatingCharList_destroy(&fchars);
    UnloadFont(data.font);
}

void handleInput() {
    updateCapsLockState();

    if (showSettings) return;

    Str *line = lines + cursorPos.y;
    int key = '\0';
    while ((key = (emulateLayout ? getEmulatedKey() : GetCharPressed())) > 0) {
        if ((key >= 33) && (key <= 126) && fitsInRect(cursorPos.x, padding, container)) {
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
    if (IsKeyPressed(KEY_TAB)) {
        const int tabSize = 8;
        const int afterTabSize = cursorPos.x % tabSize;
        cursorPos.x += tabSize - afterTabSize;
        if (!fitsInRect(cursorPos.x, padding, container))
            cursorPos.x = line->length-1;
    }
    if (isCapsLockActive()) keyResetFrames(KEY_CAPS_LOCK);
    if (IsKeyDown(KEY_LEFT_SHIFT)) keyResetFrames(KEY_LEFT_SHIFT);
    if (IsKeyDown(KEY_RIGHT_SHIFT)) keyResetFrames(KEY_RIGHT_SHIFT);
}

void updatePositions() {
    static int prevScreenWidth = 0;
    static int prevScreenHeight = 0;
    prevScreenWidth = screenWidth;
    prevScreenHeight = screenHeight;
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    static float mousePressedMove = 0;
    static bool carretPressed = false;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (carretPressed || CheckCollisionPointRec(GetMousePosition(), TRI_TO_RECT(carret)))) {
        carretPressed = true;
        mousePressedMove += GetMouseDelta().x; // multiply by charWidth to match mouse
        if (fabs(mousePressedMove) > data.charWidth + data.spacing) {
            const int cursorMove = mousePressedMove / (data.charWidth + data.spacing);
            mousePressedMove -= cursorMove * (data.charWidth + data.spacing);
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

    static float mouseScroll = 0;
    if (GetMouseWheelMoveV().y) {
        printf("%.2f\n", mouseScroll);
        mouseScroll += GetMouseWheelMoveV().y * scrollSpeed;
        if (fabs(mouseScroll) > data.fontSize + data.lineSpacing) {
            const int cursorMove = mouseScroll / (data.fontSize + data.lineSpacing);
            mouseScroll -= cursorMove * (data.fontSize + data.lineSpacing);
            int newCursorPos_y = cursorPos.y - cursorMove;
            if (newCursorPos_y <= 0) newCursorPos_y = 0;
            else if (newCursorPos_y >= rows) newCursorPos_y = rows-1;
            cursorPos.y = newCursorPos_y;
        }
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        const Vector2 mouseDelta = GetMouseDelta();
        if (cursor.x + mouseDelta.x >= padding && cursor.x + mouseDelta.x <= screenWidth - padding - data.charWidth)
            cursor.x += mouseDelta.x;
        if (cursor.y + mouseDelta.y >= padding && cursor.y + mouseDelta.y <= screenHeight - padding - data.fontSize)
            cursor.y += mouseDelta.y;
    }

    if (prevScreenWidth != screenWidth || prevScreenHeight != screenHeight) {
        cursor.x += (screenWidth - prevScreenWidth)/2.0;
        cursor.y += (screenHeight - prevScreenHeight)/2.0;
    }

    container.x = (cursor.x - padding) - (textSize(cursorPos.x, HORIZONTAL));
    container.y = (cursor.y - padding) - (textSize(cursorPos.y, VERTICAL));
    carret.x = container.x - border - padding - data.fontSize/(float)2;
    carret.y = cursor.y;

    keyboard.height = (keyRadius*2 + padding) * layouts[layout].nRows + padding,
    keyboard.x = (screenWidth - keyboard.width)/2.0;
}

void drawPage() {
    ClearBackground(GUI_COLOR(BACKGROUND_COLOR));
    DrawRectangle(UNPACK_RECT_PAD(container, border), GUI_COLOR(showSettings ? BORDER_COLOR_DISABLED : BORDER_COLOR_NORMAL));
    DrawRectangleRec(container, GUI_COLOR(BACKGROUND_COLOR));
    DrawRectangleRec(cursor, GUI_COLOR(TEXT_COLOR_DISABLED));
    DrawTringleTri(carret, GUI_COLOR(BORDER_COLOR_PRESSED));
    if (enableOverlapping) {
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
        const Vector2 textPos = Vector2Add(VECTOR2(container), (Vector2) {padding, textSize(i, VERTICAL) + padding});
        DrawTextEx(data.font, lines[i].str, textPos, data.fontSize, data.spacing, GUI_COLOR(TEXT_COLOR_NORMAL));
    }
}

void drawKeyEx(const char *normal, const char *shift, Vector2 center, int fontSize, float extraWidth, Color bgColor) {
    DrawCircleSector(
        Vector2Add(center, (Vector2) {-extraWidth/2.0, 0}), keyRadius,
        90, 270, 20, /* Left semicircle */
        bgColor
    );
    DrawCircleSector(
        Vector2Add(center, (Vector2) {extraWidth/2.0, 0}), keyRadius,
        270, 450, 20, /* Right semicircle */
        bgColor
    );
    DrawRectangle(
        center.x - extraWidth/2.0, center.y - keyRadius,
        extraWidth, keyRadius*2,
        bgColor
    );
    if (shift == NULL || *shift == '\0') {
        DrawText(
            normal,
            center.x - MeasureText(normal, fontSize)/2.0, center.y - fontSize/2.0,
            fontSize, GUI_COLOR(TEXT_COLOR_NORMAL)
        );
    } else {
        DrawText(shift, center.x - MeasureText(shift, fontSize)/2.0, center.y - fontSize, fontSize, GUI_COLOR(TEXT_COLOR_NORMAL));
        DrawText(normal, center.x - MeasureText(normal, fontSize)/2.0, center.y, fontSize, GUI_COLOR(TEXT_COLOR_NORMAL));
    }
}

float keyExtraWidth(int key, int fontSize) {
    float keyWidth = MeasureText("M", fontSize);
    switch (key) {
        case KEY_SPACE: return keyWidth * 30;
        case KEY_TAB: return keyWidth * 2;
        case KEY_BACKSPACE: return keyWidth * 8;
        case KEY_LEFT_SHIFT: case KEY_RIGHT_SHIFT: return keyWidth * 4;
        case KEY_CAPS_LOCK: return keyWidth * 8;
        default: return 0;
    }
}

void drawKey(int keyIdx, Vector2 center, int fontSize, unsigned char transparency) {
    // const char keyCharWidth = 0;
    const int normal = layouts[layout].keys[keyIdx].normal;
    const int shift = layouts[layout].keys[keyIdx].shift;

    // char text[16] = {};
    const float extraWidth = keyExtraWidth(normal, fontSize);

    switch (normal) {
        case KEY_SPACE: {
            drawKeyEx("SPACE", NULL, center, fontSize, extraWidth, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
        } break;
        case KEY_TAB: {
            drawKeyEx("Tab", NULL, center, fontSize, extraWidth, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
        } break;
        case KEY_BACKSPACE: {
            drawKeyEx("Backspace", NULL, center, fontSize, extraWidth, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
        } break;
        case KEY_LEFT_SHIFT: case KEY_RIGHT_SHIFT: {
            drawKeyEx("Shift", NULL, center, fontSize, extraWidth, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
        } break;
        case KEY_CAPS_LOCK: {
            drawKeyEx("Caps Lock", NULL, center, fontSize, extraWidth, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
        } break;
        default: {
            char normalBuf[] = {(char) normal, '\0'};
            char shiftBuf[2] = {(char) (normal == shift ? '\0' : shift), '\0'};
            drawKeyEx(normalBuf, shiftBuf, center, fontSize, extraWidth, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
        } break;
    }
}

// TODO: calculate ahead of time the length of each row to allow long keys at the end
void drawKeyboard() {
    if (keyboard.y == INFINITY) keyboard.y = hideKeyboard ? screenHeight : screenHeight - keyboard.height;

    // hide/show keyboard
    if (hideKeyboard) keyboard.y = Lerp(keyboard.y, screenHeight, 0.05);
    else keyboard.y = Lerp(keyboard.y, screenHeight-keyboard.height, 0.05);

    if (hideKeyboard && keyboard.y+0.1 >= screenHeight) return;

    DrawRectangle(0, keyboard.y, screenWidth, keyboard.height, GUI_COLOR(BASE_COLOR_DISABLED));
    const int nRows = layouts[layout].nRows;
    int longestRowSize = 0;
    for (int i = 0; i < nRows; i++) {
        if (layouts[layout].rows[i] > longestRowSize)
            longestRowSize = layouts[layout].rows[i];
    }
    for (int row = 0, i = 0; row < nRows; row++) {
        const int keyFontSize = keyRadius*0.67;
        float centerRelative[layouts[layout].rows[row]] = {};
        float rowWidth = 0;
        for (int column = 0; column < layouts[layout].rows[row]; column++) {
            int idx = i + column;
            const float keyWidth = keyRadius*2 + keyExtraWidth(layouts[layout].keys[idx].normal, keyFontSize);
            centerRelative[column] = rowWidth + padding + keyWidth/2.0;
            rowWidth = rowWidth + padding + keyWidth;
        }
        float rowBaseX = (screenWidth - rowWidth) / 2.0;

        for (int column = 0; column < layouts[layout].rows[row]; column++, i++) {
            Vector2 center = (Vector2) {
                rowBaseX + centerRelative[column],
                keyboard.y + padding + keyRadius + row * (keyRadius*2 + padding)
            };

            const unsigned char transparency = Lerp(0, 255, layouts[layout].frames[i]/(float)keyAnimationFrames);
            drawKey(i, center, keyFontSize, transparency);

            layouts[layout].frames[i] -= layouts[layout].frames[i] == 0 ? 0 : 1;
        }
    }
}

float getAndIncrement(float *n, float increment) {
    float copy = *n;
    *n += increment;

    return copy;
}
void drawSettings() {
    static const int settingsWidth = 220;
    static float settingsX = INFINITY;
    if (settingsX == INFINITY) settingsX = screenWidth;

    if (showSettings) settingsX = Lerp(settingsX, screenWidth-settingsWidth, 0.05);
    else settingsX = Lerp(settingsX, screenWidth, 0.05);

    
    float y = padding;

    DrawRectangle(settingsX, 0, settingsWidth, screenHeight, GUI_COLOR(BORDER_COLOR_DISABLED));
    GuiToggle(
        (Rectangle) {screenWidth-padding-30, getAndIncrement(&y, 60), 30, 30},
        GuiIconText(showSettings ? ICON_CROSS : ICON_BURGER_MENU, NULL),
        &showSettings
    );

    if (!showSettings && settingsX+0.1 >= screenWidth) return;
    DrawText("General Settings", settingsX+padding, getAndIncrement(&y, 20+padding), 20, GUI_COLOR(BACKGROUND_COLOR));
    GuiToggle(
        (Rectangle) {settingsX+padding, getAndIncrement(&y, 30+padding), settingsWidth-padding*2, 30},
        GUI_TOGGLE_TEXT(emulateLayout, "Emulate Qwerty Layout"),
        &emulateLayout
    );
    GuiToggle(
        (Rectangle) {settingsX+padding, getAndIncrement(&y, 30+padding), settingsWidth-padding*2, 30},
        GUI_TOGGLE_TEXT(hideKeyboard, "Hide Keyboard"),
        &hideKeyboard
    );
    GuiToggle(
        (Rectangle) {settingsX+padding, getAndIncrement(&y, 30+padding), settingsWidth-padding*2, 30},
        GUI_TOGGLE_TEXT(enableOverlapping, "Enable character overlapping"),
        &enableOverlapping
    );
    if (GuiButton((Rectangle) {settingsX+padding, getAndIncrement(&y, 30+padding), settingsWidth-padding*2, 30}, "Clear Floating Chars")) {
        FloatingCharList_destroy(&fchars);
        fchars.head = NULL; fchars.length = 0;
    }
    y+=20;

    static int selectedFont = 0;
    DrawText("Font Settings", settingsX+padding, getAndIncrement(&y, 20+padding), 20, GUI_COLOR(BACKGROUND_COLOR));
    {
        static int prevItem;
        prevItem = selectedFont;
        const bool itemChanged = GuiDropdownBox(
            (Rectangle) {settingsX+padding, getAndIncrement(&y, 30*3 + padding), settingsWidth-padding*2, 30},
            "Ubuntu Mono\nCourier New", &selectedFont, true
        );
        if (itemChanged && prevItem != selectedFont)
            setFont(fonts[selectedFont].path, fonts[selectedFont].fontSize);
    }
    y+=20;

    DrawText("Style Settings", settingsX+padding, getAndIncrement(&y, 20+padding), 20, GUI_COLOR(BACKGROUND_COLOR));
    {
        static int item = 0;
        static int prevItem;
        prevItem = item;
        const bool itemChanged = GuiDropdownBox(
            (Rectangle) {settingsX+padding, getAndIncrement(&y, 30*3 + padding), settingsWidth-padding*2, 30},
            "Light\nDark", &item, true
        );
        if (itemChanged && prevItem != item)
            setTheme(themes[item].path, fonts[selectedFont].path, fonts[selectedFont].fontSize);
    }
    y+=20;

    DrawText("Layout Settings", settingsX+padding, getAndIncrement(&y, 20+padding), 20, GUI_COLOR(BACKGROUND_COLOR));
    {
        static int item = 0;
        static int prevItem;
        prevItem = item;
        const bool itemChanged = GuiDropdownBox(
            (Rectangle) {settingsX+padding, getAndIncrement(&y, 30*3 + padding), settingsWidth-padding*2, 30},
            "QWERTY\nBASIC", &item, true
        );
        if (itemChanged && prevItem != item) layout = item;
    }
}

#ifdef DEBUG
void drawDebugInfo() {
    Color color = GUI_COLOR(BORDER_COLOR_FOCUSED);
    DrawLine(0, container.y, screenWidth, container.y, color);
    DrawText("X", container.x+margin, margin, 10, color);
    DrawLine(container.x, 0, container.x, screenHeight, color);
    DrawText("Y", margin, container.y+margin, 10, color);
    char pos[64] = {};
    snprintf(pos, sizeof(pos), "%.2f, %.2f", UNPACK_VECTOR2(GetMousePosition()));
    DrawText(pos, UNPACK_VECTOR2(Vector2Add(GetMousePosition(), (Vector2) {5,-5})), 10, color);
}
#endif /* ifdef DEBUG */

int main() {
    init();

    while (!WindowShouldClose()) {
        handleInput();
        updatePositions();

        BeginDrawing();
        {
            drawPage();
            drawKeyboard();
            drawSettings();
            #ifdef DEBUG
            drawDebugInfo();
            #endif /* ifdef DEBUG */
        }
        EndDrawing();
    }

    closeAndFree();
    CloseWindow();
    return 0;
}
