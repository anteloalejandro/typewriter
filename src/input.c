#include "../raylib/include/raygui.h"
#include <ctype.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

#define MAX_KEYS 128
#define LARGEST_KEY KEY_KB_MENU

typedef struct {
    int code;
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
            {KEY_GRAVE, KEY_GRAVE, '~'}, {KEY_ONE, KEY_ONE, '!'}, {KEY_TWO, KEY_TWO, '@'}, {KEY_THREE, KEY_THREE, '#'}, {KEY_FOUR, KEY_FOUR, '$'}, {KEY_FIVE, KEY_FIVE, '%'}, {KEY_SIX, KEY_SIX, '^'}, {KEY_SEVEN, KEY_SEVEN, '&'}, {KEY_EIGHT, KEY_EIGHT, '*'}, {KEY_NINE, KEY_NINE, '('}, {KEY_ZERO, KEY_ZERO, ')'}, {KEY_MINUS, KEY_MINUS, '_'}, {KEY_EQUAL, KEY_EQUAL, '+'}, {KEY_BACKSPACE, KEY_BACKSPACE, KEY_BACKSPACE},
            {KEY_TAB, KEY_TAB, KEY_TAB}, {KEY_Q, KEY_Q, KEY_Q}, {KEY_W, KEY_W, KEY_W}, {KEY_E, KEY_E, KEY_E}, {KEY_R, KEY_R, KEY_R}, {KEY_T, KEY_T, KEY_T}, {KEY_Y, KEY_Y, KEY_Y}, {KEY_U, KEY_U, KEY_U}, {KEY_I, KEY_I, KEY_I}, {KEY_O, KEY_O, KEY_O}, {KEY_P, KEY_P, KEY_P}, {KEY_BACKSLASH, KEY_BACKSLASH, '|'},
            {KEY_CAPS_LOCK, KEY_CAPS_LOCK, KEY_CAPS_LOCK}, {KEY_A, KEY_A, KEY_A}, {KEY_S, KEY_S, KEY_S}, {KEY_D, KEY_D, KEY_D}, {KEY_F, KEY_F, KEY_F}, {KEY_G, KEY_G, KEY_G}, {KEY_H, KEY_H, KEY_H}, {KEY_J, KEY_J, KEY_J}, {KEY_K, KEY_K, KEY_K}, {KEY_L, KEY_L, KEY_L}, {KEY_SEMICOLON, KEY_SEMICOLON, ':'}, {KEY_APOSTROPHE, KEY_APOSTROPHE, '"'},
            {KEY_LEFT_SHIFT, KEY_LEFT_SHIFT, KEY_LEFT_SHIFT}, {KEY_Z, KEY_Z, KEY_Z}, {KEY_X, KEY_X, KEY_X}, {KEY_C, KEY_C, KEY_C}, {KEY_V, KEY_V, KEY_V}, {KEY_B, KEY_B, KEY_B}, {KEY_N, KEY_N, KEY_N}, {KEY_M, KEY_M, KEY_M}, {KEY_COMMA, KEY_COMMA, '<'}, {KEY_PERIOD, KEY_PERIOD, '>'}, {KEY_SLASH, KEY_SLASH, '?'} ,{KEY_RIGHT_SHIFT, KEY_RIGHT_SHIFT, KEY_RIGHT_SHIFT},
            {KEY_SPACE, KEY_SPACE, KEY_SPACE},
        },
        .frames = {},
        .rows = { 14, 12, 12, 12, 1 },
        .nRows = 5
    },
    {
        .keys = {
            {KEY_ONE, KEY_ONE, KEY_ONE}, {KEY_TWO, KEY_TWO, KEY_TWO}, {KEY_THREE, KEY_THREE, KEY_THREE}, {KEY_FOUR, KEY_FOUR, KEY_FOUR}, {KEY_FIVE, KEY_FIVE, KEY_FIVE}, {KEY_SIX, KEY_SIX, KEY_SIX}, {KEY_SEVEN, KEY_SEVEN, KEY_SEVEN}, {KEY_EIGHT, KEY_EIGHT, KEY_EIGHT}, {KEY_NINE, KEY_NINE, KEY_NINE}, {KEY_ZERO, KEY_ZERO, KEY_ZERO},
            {KEY_Q, KEY_Q, KEY_Q}, {KEY_W, KEY_W, KEY_W}, {KEY_E, KEY_E, KEY_E}, {KEY_R, KEY_R, KEY_R}, {KEY_T, KEY_T, KEY_T}, {KEY_Y, KEY_Y, KEY_Y}, {KEY_U, KEY_U, KEY_U}, {KEY_I, KEY_I, KEY_I}, {KEY_O, KEY_O, KEY_O}, {KEY_P, KEY_P, KEY_P},
            {KEY_A, KEY_A, KEY_A}, {KEY_S, KEY_S, KEY_S}, {KEY_D, KEY_D, KEY_D}, {KEY_F, KEY_F, KEY_F}, {KEY_G, KEY_G, KEY_G}, {KEY_H, KEY_H, KEY_H}, {KEY_J, KEY_J, KEY_J}, {KEY_K, KEY_K, KEY_K}, {KEY_L, KEY_L, KEY_L},
            {KEY_Z, KEY_Z, KEY_Z}, {KEY_X, KEY_X, KEY_X}, {KEY_C, KEY_C, KEY_C}, {KEY_V, KEY_V, KEY_V}, {KEY_B, KEY_B, KEY_B}, {KEY_N, KEY_N, KEY_N}, {KEY_M, KEY_M, KEY_M},
            {KEY_SPACE, KEY_SPACE, KEY_SPACE},
        },
        .frames = {},
        .rows = {10,10,9,7,1},
        .nRows = 5
    },
};

#define N_LAYOUTS sizeof(layouts)/sizeof(KeyboardLayout)

static int codeLookupTable[N_LAYOUTS][LARGEST_KEY+1] = {};
static int normalLookupTable[N_LAYOUTS][LARGEST_KEY+1] = {};
static char shiftLookupTable[N_LAYOUTS][LARGEST_KEY+1] = {};

const int keyAnimationFrames = 10;

void fillCodeLookupTable() {
    const int n = N_LAYOUTS;
    for (int i = 0; i < n; i++) {
        int code;
        for (int j = 0; j < MAX_KEYS && (code = layouts[i].keys[j].code) != 0; j++) {
            normalLookupTable[i][code] = j;
        }
    }
}

void fillNormalLookupTable() {
    const int n = N_LAYOUTS;
    for (int i = 0; i < n; i++) {
        int normal;
        for (int j = 0; j < MAX_KEYS && (normal = layouts[i].keys[j].normal) != 0; j++) {
            normalLookupTable[i][normal] = j;
        }
    }
}

void fillShiftLookupTable() {
    const int n = N_LAYOUTS;
    for (int i = 0; i < n; i++) {
        int shift;
        for (int j = 0; j < MAX_KEYS && (shift = layouts[i].keys[j].shift) != 0; j++) {
            shiftLookupTable[i][shift] = j;
        }
    }
}

// TODO: fill table on another thread when the program starts
int getKeyIndex(int keyCode) {
    static bool filled = false;
    if (!filled) {
        fillCodeLookupTable();
        filled = true;
    }

    if (keyCode > LARGEST_KEY) return -1;
    if (keyCode == layouts[layout].keys[0].code) return 0;

    const int code = normalLookupTable[layout][keyCode];
    return code ? code : -1;
}
int getKeyNormal(int keyCode) {
    int i = getKeyIndex(keyCode);
    if (i == -1) return -1;
    return layouts[layout].keys[i].normal;
}
int getKeyShift(int keyCode) {
    int i = getKeyIndex(keyCode);
    if (i == -1) return -1;
    return layouts[layout].keys[i].shift;
}
int getKeyUnshift(int keyShift) {
    static bool filled = false;
    if (!filled) {
        fillShiftLookupTable();
        filled = true;
    }

    if (keyShift == layouts[layout].keys[0].shift) return layouts[layout].keys[0].normal;
    int i = shiftLookupTable[layout][keyShift];

    if (i == 0) return -1;

    return layouts[layout].keys[i].normal;
}
int getKeyCode(int keyNormal) {
    static bool filled = false;
    if (!filled) {
        fillNormalLookupTable();
        filled = true;
    }

    if (keyNormal == layouts[layout].keys[0].normal) return layouts[layout].keys[0].code;
    int i = codeLookupTable[layout][keyNormal];
    
    if (i == 0) return -1;

    return layouts[layout].keys[i].code;
}

void keyResetFrames(int keyCode) {
    const int key = getKeyNormal(keyCode);
    const int i = getKeyIndex(key);
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
    const int code = GetKeyPressed();
    if (getKeyIndex(code) == -1) return -1;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        return getKeyShift(code);
    } else if (!isCapsLockActive()) {
        return tolower(getKeyNormal(code));
    }
    
    return getKeyNormal(code);
}
