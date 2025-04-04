#include "raylib/include/raylib.h"
#include "raylib/include/raymath.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
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
#define VECTOR2(shape) CLITERAL(Vector2) { (shape.x), (shape.y) }
#define TRI_TO_RECT(tri) CLITERAL(Rectangle) {carret.x, carret.y, carret.c.x, carret.b.y}
#define GUI_COLOR(color) GetColor(GuiGetStyle(DEFAULT, color))
#define GUI_TOGGLE_TEXT(isTrue, str) GuiIconText((isTrue) ? ICON_BOX_CENTER : ICON_BOX, str)
#define TRANSPARENTIZE(color, alpha) CLITERAL(Color) {color.r, color.g, color.b, alpha}

struct {
    char *name;
    char *path;
    int fontSize;
} fonts[] = {
    { "Ubuntu Mono", "resources/UbuntuMono-R.ttf", 20 },
    { "Courier New", "resources/cour.ttf", 20 },
};
struct {
    char *name;
    char *path;
} themes[] = {
    { "Light", NULL },
    { "Dark", "resources/style_dark.rgs" }
};

int screenWidth = 1200;
int screenHeight = 800;

bool showSettings = false;
bool forceLayout = false;
bool hideKeyboard = false;
bool enableOverlapping = false;

const int margin = 20;
const int padding = 5;
const int border = 3;
const int rows = 20;
const int cols = 80;
const int scrollSpeed = 10;
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

    const int nRows = sizeof(keyboardKeys.rows)/sizeof(int);
    const int maxCols = 10;
    keyboard = (Rectangle) {
        .height = (keyRadius*2 + padding) * nRows + padding,
        .width = (keyRadius*2 + padding) * maxCols + padding,
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
    if (!showSettings) {
        Str *line = lines + cursorPos.y;
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
}

void updatePositions() {
    static int prevScreenWidth = 0;
    static int prevScreenHeight = 0;
    prevScreenWidth = screenWidth;
    prevScreenHeight = screenHeight;
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

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

void drawKeyboard() {
    static int frames = 0;
    const int keyboardFrames = 5;
    if (hideKeyboard) frames++;
    else frames--;
    frames = Clamp(frames, 0, keyboardFrames);

    keyboard.y = Lerp(screenHeight-keyboard.height, screenHeight, frames/(float)keyboardFrames);
    if (!hideKeyboard || frames < keyboardFrames) {
        DrawRectangle(0, keyboard.y, screenWidth, keyboard.height, GUI_COLOR(BASE_COLOR_DISABLED));
        DrawRectangleRec(keyboard, GUI_COLOR(BASE_COLOR_DISABLED));
        const int nRows = sizeof(keyboardKeys.rows)/sizeof(int);
        for (int row = 0, i = 0; row < nRows; row++) {
            const float columnOffset = (keyboardKeys.rows[0]-keyboardKeys.rows[row])/2.0;
            for (int column = 0; column < keyboardKeys.rows[row]; column++, i++) {
                Vector2 center = (Vector2) {
                    keyboard.x + padding + keyRadius + (column+columnOffset) * (keyRadius*2 + padding),
                    keyboard.y + padding + keyRadius + row * (keyRadius*2 + padding)
                };

                char buf[] = {(char) keyboardKeys.keys[i], '\0'};
                const int keyFontSize = keyRadius*0.67;
                const int keyCharWidth = MeasureText(buf, keyFontSize);

                const unsigned char transparency = Lerp(0, 255, keyboardKeys.frames[i]/(float)keyboardKeys.initialFrames);
                if (keyboardKeys.keys[i] == KEY_SPACE) {
                    const int spacebarWidth = keyCharWidth * 30;
                    DrawCircleSector(
                        Vector2Add(center, (Vector2) {-spacebarWidth/2.0, 0}), keyRadius,
                        90, 270, 20, /* Left semicircle */
                        TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency)
                    );
                    DrawCircleSector(
                        Vector2Add(center, (Vector2) {spacebarWidth/2.0, 0}), keyRadius,
                        270, 450, 20, /* Right semicircle */
                        TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency)
                    );
                    DrawRectangle(
                        center.x - spacebarWidth/2.0, center.y - keyRadius,
                        spacebarWidth, keyRadius*2,
                        TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency)
                    );
                    DrawText(
                        "SPACE",
                        center.x - MeasureText("SPACE", keyFontSize)/2.0, center.y - keyFontSize/2.0,
                        keyFontSize, GUI_COLOR(TEXT_COLOR_NORMAL)
                    );
                } else {
                    DrawCircleV(center, keyRadius, TRANSPARENTIZE(GUI_COLOR(BACKGROUND_COLOR), 255 - transparency));
                    DrawText(buf, center.x - keyCharWidth/(float)2, center.y - keyFontSize/(float)2, keyFontSize, GUI_COLOR(TEXT_COLOR_NORMAL));
                }

                keyboardKeys.frames[i] -= keyboardKeys.frames[i] == 0 ? 0 : 1;
            }
        }
    }
}

float getAndIncrement(float *n, float increment) {
    float copy = *n;
    *n += increment;

    return copy;
}
void drawSettings() {
    const int settingsWidth = 220;
    const int finalSettingsX = screenWidth-settingsWidth;
    const int settingsFrames = 10;
    static int frames = 0;
    if (showSettings) frames++;
    else frames--;
    frames = Clamp(frames, 0, settingsFrames);

    const int settingsX = Lerp(screenWidth, finalSettingsX, frames/(float)settingsFrames);
    float y = padding;

    DrawRectangle(settingsX, 0, settingsWidth, screenHeight, GUI_COLOR(BORDER_COLOR_DISABLED));
    GuiToggle(
        (Rectangle) {screenWidth-padding-30, getAndIncrement(&y, 60), 30, 30},
        GuiIconText(showSettings ? ICON_CROSS : ICON_BURGER_MENU, NULL),
        &showSettings
    );

    DrawText("General Settings", settingsX+padding, getAndIncrement(&y, 20+padding), 20, GUI_COLOR(BACKGROUND_COLOR));
    GuiToggle(
        (Rectangle) {settingsX+padding, getAndIncrement(&y, 30+padding), settingsWidth-padding*2, 30},
        GUI_TOGGLE_TEXT(forceLayout, "Force Typewriter Layout"),
        &forceLayout
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
            "Modern\nClassic", &selectedFont, true
        );
        if (itemChanged && prevItem != selectedFont) setFont(fonts[selectedFont].path, fonts[selectedFont].fontSize);
        selectedFont = selectedFont;
    }
    y+=20;

    DrawText("Style Settings", settingsX+padding, getAndIncrement(&y, 20+padding), 20, GUI_COLOR(BACKGROUND_COLOR));
    {
        static int item = 0;
        static int prevItem;
        prevItem = item;
        char buf[64];
        const bool itemChanged = GuiDropdownBox(
            (Rectangle) {settingsX+padding, getAndIncrement(&y, 30*3 + padding), settingsWidth-padding*2, 30},
            "Light\nDark", &item, true
        );
        if (itemChanged && prevItem != item)
            setTheme(themes[item].path, fonts[selectedFont].path, fonts[selectedFont].fontSize);
    }
}

void drawDebugInfo() {
    Color color = GUI_COLOR(BORDER_COLOR_FOCUSED);
    DrawLine(0, container.y, screenWidth, container.y, color);
    DrawText("X", container.x+margin, margin, 10, color);
    DrawLine(container.x, 0, container.x, screenHeight, color);
    DrawText("Y", margin, container.y+margin, 10, color);
}

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
            drawDebugInfo();
        }
        EndDrawing();
    }

    closeAndFree();
    CloseWindow();
    return 0;
}
