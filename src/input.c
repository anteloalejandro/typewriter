#include "../raylib/include/raygui.h"
#include <ctype.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <string.h>

// STATIC VARIABLES
#define QWERTY_KEYS \
X(0, KEY_ONE, '!') X(1, KEY_TWO, '@') X(2, KEY_THREE, '#') X(3, KEY_FOUR, '$') X(4, KEY_FIVE, '%') X(5, KEY_SIX, '^') X(6, KEY_SEVEN, '&') X(7, KEY_EIGHT, '*') X(8, KEY_NINE, '(') X(9, KEY_ZERO, ')') X(10, KEY_MINUS, '_') X(11, KEY_EQUAL, '+') \
X(12, KEY_Q, KEY_Q) X(13, KEY_W, KEY_W) X(14, KEY_E, KEY_E) X(15, KEY_R, KEY_R) X(16, KEY_T, KEY_T) X(17, KEY_Y, KEY_Y) X(18, KEY_U, KEY_U) X(19, KEY_I, KEY_I) X(20, KEY_O, KEY_O) X(21, KEY_P, KEY_P) \
X(22, KEY_A, KEY_A) X(23, KEY_S, KEY_S) X(24, KEY_D, KEY_D) X(25, KEY_F, KEY_F) X(26, KEY_G, KEY_G) X(27, KEY_H, KEY_H) X(28, KEY_J, KEY_J) X(29, KEY_K, KEY_K) X(30, KEY_L, KEY_L) X(31, KEY_SEMICOLON, ':') X(32, KEY_APOSTROPHE, '"') \
X(33, KEY_Z, KEY_Z) X(34, KEY_X, KEY_X) X(35, KEY_C, KEY_C) X(36, KEY_V, KEY_V) X(37, KEY_B, KEY_B) X(38, KEY_N, KEY_N) X(39, KEY_M, KEY_M) X(40, KEY_COMMA, '<') X(41, KEY_PERIOD, '>')\
X(42, KEY_SPACE, KEY_SPACE)

#define BASIC_KEYS \
X(0, KEY_ONE, KEY_ONE) X(1, KEY_TWO, KEY_TWO) X(2, KEY_THREE, KEY_THREE) X(3, KEY_FOUR, KEY_FOUR) X(4, KEY_FIVE, KEY_FIVE) X(5, KEY_SIX, KEY_SIX) X(6, KEY_SEVEN, KEY_SEVEN) X(7, KEY_EIGHT, KEY_EIGHT) X(8, KEY_NINE, KEY_NINE) X(9, KEY_ZERO, KEY_ZERO) \
X(10, KEY_Q, KEY_Q) X(11, KEY_W, KEY_W) X(12, KEY_E, KEY_E) X(13, KEY_R, KEY_R) X(14, KEY_T, KEY_T) X(15, KEY_Y, KEY_Y) X(16, KEY_U, KEY_U) X(17, KEY_I, KEY_I) X(18, KEY_O, KEY_O) X(19, KEY_P, KEY_P) \
X(20, KEY_A, KEY_A) X(21, KEY_S, KEY_S) X(22, KEY_D, KEY_D) X(23, KEY_F, KEY_F) X(24, KEY_G, KEY_G) X(25, KEY_H, KEY_H) X(26, KEY_J, KEY_J) X(27, KEY_K, KEY_K) X(28, KEY_L, KEY_L) \
X(29, KEY_Z, KEY_Z) X(30, KEY_X, KEY_X) X(31, KEY_C, KEY_C) X(32, KEY_V, KEY_V) X(33, KEY_B, KEY_B) X(34, KEY_N, KEY_N) X(35, KEY_M, KEY_M) \
X(36, KEY_SPACE, KEY_SPACE)

typedef struct {
    int *keys;
    int *shiftKeys;
    int *frames;
    int *rows;
    int nRows, nKeys;
} KeyboardLayout;

enum Layouts { QWERTY = 0, BASIC };
KeyboardLayout qwertyLayout;
KeyboardLayout basicLayout;
KeyboardLayout *layouts[] = {&qwertyLayout, &basicLayout};
enum Layouts layout = QWERTY;

const int keyAnimationFrames = 10;

void initKeyboardLayouts() {
    #define INIT_KEYBOARD_PROP(ptr, layout, property, length) \
        layout.property = ptr; \
        ptr += length; \
        memcpy(layout.property, property, sizeof(int)*length); \
    
    {
        int keys[] = {
            #define X(i, key, shift) key,
            QWERTY_KEYS
            #undef X
        };
        int shiftKeys[] = {
            #define X(i, key, shift) shift,
            QWERTY_KEYS
            #undef X
        };
        int frames[] = {
            #define X(...) 0,
            QWERTY_KEYS
            #undef X
        };
        int rows[] = { 12, 10, 11, 9, 1 };
        qwertyLayout.nKeys = 
            #define X(...) +1
            QWERTY_KEYS
            #undef X
        ;
        qwertyLayout.nRows = sizeof(rows)/sizeof(int);

        int *arena = malloc(sizeof(int)*(qwertyLayout.nKeys*3 + qwertyLayout.nKeys));
        INIT_KEYBOARD_PROP(arena, qwertyLayout, keys, qwertyLayout.nKeys);
        INIT_KEYBOARD_PROP(arena, qwertyLayout, shiftKeys, qwertyLayout.nKeys);
        INIT_KEYBOARD_PROP(arena, qwertyLayout, frames, qwertyLayout.nKeys);
        INIT_KEYBOARD_PROP(arena, qwertyLayout, rows, qwertyLayout.nRows);
    }

    {
        int keys[] = {
            #define X(i, key, shift) key,
            BASIC_KEYS
            #undef X
        };
        int shiftKeys[] = {
            #define X(i, key, shift) shift,
            BASIC_KEYS
            #undef X
        };
        int frames[] = {
            #define X(...) 0,
            BASIC_KEYS
            #undef X
        };
        int rows[] = { 10, 10, 9, 7, 1 };
        basicLayout.nKeys = 
            #define X(...) +1
            BASIC_KEYS
            #undef X
        ;
        basicLayout.nRows = sizeof(rows)/sizeof(int);

        int *arena = malloc(sizeof(int)*(basicLayout.nKeys*3 + basicLayout.nKeys));
        INIT_KEYBOARD_PROP(arena, basicLayout, keys, basicLayout.nKeys);
        INIT_KEYBOARD_PROP(arena, basicLayout, shiftKeys, basicLayout.nKeys);
        INIT_KEYBOARD_PROP(arena, basicLayout, frames, basicLayout.nKeys);
        INIT_KEYBOARD_PROP(arena, basicLayout, rows, basicLayout.nRows);
    }

    #undef INIT_KEYBOARD_PROP
}

void freeKeyboardLayouts() {
    free(qwertyLayout.keys);
    free(basicLayout.keys);
}

int getKeysIndex(int key) {
    switch (layout) {
        case QWERTY: {
            switch (key) {
                #define X(i, name, shift) case name: return i;
                QWERTY_KEYS
                #undef X
                default: return -1;
            }
        } break;
        case BASIC: {
            switch (key) {
                #define X(i, name, shift) case name: return i;
                BASIC_KEYS
                #undef X
                default: return -1;
            }
        } break;
    }

    return -1;
}
int getKeyShift(int key) {
    int i = getKeysIndex(key);
    if (i == -1) return -1;
    return layouts[layout]->shiftKeys[i];
}
int getKeyNoShift(int c) {
    if (getKeysIndex(c) != -1) return c;
    switch (layout) {
        case QWERTY: {
            switch (c) {
                #define X(i, key, shift) case shift: return key;
                QWERTY_KEYS
                #undef X
                default: return -1;
            }
        } break;
        case BASIC: {
            switch (c) {
                #define X(i, key, shift) case shift: return key;
                BASIC_KEYS
                #undef X
                default: return -1;
            }
        } break;
    }

    return -1;
}

void keyResetFrames(int key) {
    int i = getKeysIndex(getKeyNoShift(key));
    if (i == -1) return;
    layouts[layout]->frames[i] = keyAnimationFrames;
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

int getForceLayoutKey() {
    int c = GetKeyPressed();
    const bool caps_lock = isCapsLockActive(); 
    if (getKeysIndex(c) == -1) return -1;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        c = getKeyShift(c);
    } else if (!caps_lock) {
        c = tolower(c);
    }
    return c;
}

#undef QWERTY_KEYS
