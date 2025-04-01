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
#define VECTOR2_RECT(rect) CLITERAL(Vector2) { (rect.x), (rect.y) }
#define VECTOR2_RECT_PAD(rect, n) CLITERAL(Vector2) { (rect.x) + (n), (rect.y) + (n) }
#define TRI_TO_RECT(tri) CLITERAL(Rectangle) {carret.x, carret.y, carret.c.x, carret.b.y}
#define GUI_COLOR(color) GetColor(GuiGetStyle(DEFAULT, color))
#define GUI_ICON(icon) "#" #icon "#"
#define TRANSPARENTIZE(color, alpha) CLITERAL(Color) {color.r, color.g, color.b, alpha}

int main()
{
    int screenWidth = 1200;
    int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "Typewriter");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    SetTargetFPS(60);

    SetRandomSeed(time(NULL));

    bool showSettings = false;
    bool forceLayout = false;
    bool hideKeyboard = false;
    bool showFloatingChars = false;

    setFont(fonts[0].path, fonts[0].fontSize);

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

    Str lines[rows];
    for (int i = 0; i < rows; i++) {
        lines[i].str = NULL;
        lines[i].length = 0;

        char buf[cols+1];
        snprintf(buf, cols, "%*s", cols, "");
        Str_append(lines+i, buf, cols);
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
                        const float columnOffset = (keyboardKeys.rows[0]-keyboardKeys.rows[row])/2.0;
                        for (int column = 0; column < keyboardKeys.rows[row]; column++, i++) {

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
                DrawText("General Settings", settingsX+padding, 60, 20, GUI_COLOR(BACKGROUND_COLOR));
                GuiToggle(
                    (Rectangle) {settingsX+padding, 90, settingsWidth-padding*2, 30},
                    forceLayout ? GUI_ICON(89) "Force Typewriter Layout" : GUI_ICON(80) "Force Typewriter Layout",
                    &forceLayout
                );
                GuiToggle(
                    (Rectangle) {settingsX+padding, 130, settingsWidth-padding*2, 30},
                    hideKeyboard ? GUI_ICON(89) "Hide Keyboard" : GUI_ICON(80) "Hide Keyboard",
                    &hideKeyboard
                );
                GuiToggle(
                    (Rectangle) {settingsX+padding, 170, settingsWidth-padding*2, 30},
                    showFloatingChars ? GUI_ICON(89) "Enable character overlapping" : GUI_ICON(80) "Enable character overlapping",
                    &showFloatingChars
                );
                if (GuiButton((Rectangle) {settingsX+padding, 210, settingsWidth-padding*2, 30}, "Clear Floating Chars")) {
                    FloatingCharList_destroy(&fchars);
                    fchars.head = NULL; fchars.length = 0;
                }

                DrawText("Font Settings", settingsX+padding, 260, 20, GUI_COLOR(BACKGROUND_COLOR));
                static int item = 0;
                bool fontChanged = GuiDropdownBox(
                    (Rectangle) {settingsX+padding, 290, settingsWidth-padding*2, 30},
                    "Modern\nClassic",
                    &item,
                    true
                );
                if (fontChanged) setFont(fonts[item].path, fonts[item].fontSize);
            }

            GuiToggle((Rectangle){screenWidth-padding-30, padding, 30, 30}, showSettings ? GUI_ICON(113) : GUI_ICON(214), &showSettings);

            DrawLine(0, container.y, screenWidth, container.y, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawText("X", container.x+margin, margin, 10, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawLine(container.x, 0, container.x, screenHeight, GUI_COLOR(BORDER_COLOR_FOCUSED));
            DrawText("Y", margin, container.y+margin, 10, GUI_COLOR(BORDER_COLOR_FOCUSED));
        }
        EndDrawing();
    }

    for (int i = 0; i < rows; i++) Str_destroy(lines+i);
    FloatingCharList_destroy(&fchars);
    UnloadFont(data.font);
    CloseWindow();
    return 0;
}
