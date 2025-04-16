#include "../raylib/include/raygui.h"
#include <ctype.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define MAX_KEYS 128
#define LARGEST_KEY KEY_KB_MENU

typedef struct {
    int normal;
    int shift;
} KeyPair;

typedef struct {
    KeyPair keys[MAX_KEYS];
    int frames[MAX_KEYS];
    int rows[7];
    int nRows;
} KeyboardLayout;

enum { QWERTY = 0, BASIC } layout = QWERTY;
KeyboardLayout layouts[] = {
    {
        .keys = {
            {KEY_GRAVE, '~'}, {KEY_ONE, '!'}, {KEY_TWO, '@'}, {KEY_THREE, '#'}, {KEY_FOUR, '$'}, {KEY_FIVE, '%'}, {KEY_SIX, '^'}, {KEY_SEVEN, '&'}, {KEY_EIGHT, '*'}, {KEY_NINE, '('}, {KEY_ZERO, ')'}, {KEY_MINUS, '_'}, {KEY_EQUAL, '+'}, {KEY_BACKSPACE, KEY_BACKSPACE},
            {KEY_TAB, KEY_TAB}, {KEY_Q, KEY_Q}, {KEY_W, KEY_W}, {KEY_E, KEY_E}, {KEY_R, KEY_R}, {KEY_T, KEY_T}, {KEY_Y, KEY_Y}, {KEY_U, KEY_U}, {KEY_I, KEY_I}, {KEY_O, KEY_O}, {KEY_P, KEY_P}, {KEY_BACKSLASH, '|'},
            {KEY_CAPS_LOCK, KEY_CAPS_LOCK}, {KEY_A, KEY_A}, {KEY_S, KEY_S}, {KEY_D, KEY_D}, {KEY_F, KEY_F}, {KEY_G, KEY_G}, {KEY_H, KEY_H}, {KEY_J, KEY_J}, {KEY_K, KEY_K}, {KEY_L, KEY_L}, {KEY_SEMICOLON, ':'}, {KEY_APOSTROPHE, '"'},
            {KEY_LEFT_SHIFT, KEY_LEFT_SHIFT}, {KEY_Z, KEY_Z}, {KEY_X, KEY_X}, {KEY_C, KEY_C}, {KEY_V, KEY_V}, {KEY_B, KEY_B}, {KEY_N, KEY_N}, {KEY_M, KEY_M}, {KEY_COMMA, '<'}, {KEY_PERIOD, '>'}, {KEY_SLASH, '?'} ,{KEY_RIGHT_SHIFT, KEY_RIGHT_SHIFT},
            {KEY_SPACE, KEY_SPACE},
        },
        .frames = {},
        .rows = { 14, 12, 12, 12, 1 },
        .nRows = 5
    },
    {
        .keys = {
            {KEY_ONE, KEY_ONE}, {KEY_TWO, KEY_TWO}, {KEY_THREE, KEY_THREE}, {KEY_FOUR, KEY_FOUR}, {KEY_FIVE, KEY_FIVE}, {KEY_SIX, KEY_SIX}, {KEY_SEVEN, KEY_SEVEN}, {KEY_EIGHT, KEY_EIGHT}, {KEY_NINE, KEY_NINE}, {KEY_ZERO, KEY_ZERO},
            {KEY_Q, KEY_Q}, {KEY_W, KEY_W}, {KEY_E, KEY_E}, {KEY_R, KEY_R}, {KEY_T, KEY_T}, {KEY_Y, KEY_Y}, {KEY_U, KEY_U}, {KEY_I, KEY_I}, {KEY_O, KEY_O}, {KEY_P, KEY_P},
            {KEY_A, KEY_A}, {KEY_S, KEY_S}, {KEY_D, KEY_D}, {KEY_F, KEY_F}, {KEY_G, KEY_G}, {KEY_H, KEY_H}, {KEY_J, KEY_J}, {KEY_K, KEY_K}, {KEY_L, KEY_L},
            {KEY_Z, KEY_Z}, {KEY_X, KEY_X}, {KEY_C, KEY_C}, {KEY_V, KEY_V}, {KEY_B, KEY_B}, {KEY_N, KEY_N}, {KEY_M, KEY_M},
            {KEY_SPACE, KEY_SPACE},
        },
        .frames = {},
        .rows = {10,10,9,7,1},
        .nRows = 5
    },
};

int normalLookupTable[sizeof(layouts)/sizeof(KeyboardLayout)][LARGEST_KEY+1] = {};
char shiftLookupTable[sizeof(layouts)/sizeof(KeyboardLayout)][LARGEST_KEY+1] = {};

const int keyAnimationFrames = 10;

void fillNormalLookupTable() {
    int n = sizeof(layouts)/sizeof(KeyboardLayout);
    for (int i = 0; i < n; i++) {
        int normal;
        for (int j = 0; j < MAX_KEYS && (normal = layouts[i].keys[j].normal) != 0; j++) {
            normalLookupTable[i][normal] = j;
        }
    }
}

void fillShiftLookupTable() {
    int n = sizeof(layouts)/sizeof(KeyboardLayout);
    for (int i = 0; i < n; i++) {
        int shift;
        for (int j = 0; j < MAX_KEYS && (shift = layouts[i].keys[j].shift) != 0; j++) {
            shiftLookupTable[i][shift] = j;
        }
    }
}

int getKeysIndex(int key) {
    static bool filled = false;
    if (!filled) {
        fillNormalLookupTable();
        filled = true;
    }

    if (key > LARGEST_KEY) return -1;
    if (key == layouts[layout].keys[0].normal) return 0;

    const int normal = normalLookupTable[layout][key];
    return normal ? normal : -1;
}
int getKeyShift(int key) {
    int i = getKeysIndex(key);
    if (i == -1) return -1;
    return layouts[layout].keys[i].shift;
}
int getKeyNormal(int c) {
    static bool filled = false;
    if (!filled) {
        fillShiftLookupTable();
        filled = true;
    }

    if (c == layouts[layout].keys[0].shift) return layouts[layout].keys[0].normal;

    int i = getKeysIndex(c);
    if (i != -1) return c;

    i = shiftLookupTable[layout][c];

    if (i == 0) return -1;

    return layouts[layout].keys[i].normal;
}

void keyResetFrames(int key) {
    key = getKeyNormal(key);
    int i = getKeysIndex(key);
    if (i == -1) return;
    layouts[layout].frames[i] = keyAnimationFrames;
}

// Figure out wether the caps lock is active or not using GLFW, based on the previous state.
// Needs to be run every frame to update properly.
// Assumes it's off by default.
// NOTE: CAPS_LOCK is broken on raylib and will always only be detected as DOWN once pressed,
// and there is no way to change it.
// <https://github.com/raysan5/raylib/issues/4078>
bool isCapsLockActive() {
    static bool lastState = false;
    static bool isActive = false;

    static bool isGlfwContextSet = false;
    if (!isGlfwContextSet) {
        glfwMakeContextCurrent(GetWindowHandle());
        isGlfwContextSet = true;
    }

    GLFWwindow *w = glfwGetCurrentContext();
    const bool currentState = glfwGetKey(w, GLFW_KEY_CAPS_LOCK);

    if (currentState && !lastState) isActive = !isActive;
    lastState = currentState;

    return isActive;
}
void updateCapsLockState() {
    isCapsLockActive();
}

int getEmulatedKey() {
    int c = GetKeyPressed();
    if (getKeysIndex(c) == -1) return -1;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        c = getKeyShift(c);
    } else if (!isCapsLockActive()) {
        c = tolower(c);
    }
    return c;
}
