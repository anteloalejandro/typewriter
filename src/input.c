#include "../raylib/include/raygui.h"
#include <ctype.h>
#include <GLFW/glfw3.h>

// STATIC VARIABLES
#define KEYBOARD_KEYS \
X(0, KEY_ONE, '!') X(1, KEY_TWO, '"') X(2, KEY_THREE, '#') X(3, KEY_FOUR, '$') X(4, KEY_FIVE, '%') X(5, KEY_SIX, '_') X(6, KEY_SEVEN, '&') X(7, KEY_EIGHT, '\'') X(8, KEY_NINE, '(') X(9, KEY_ZERO, ')') \
X(10, KEY_Q, KEY_Q) X(11, KEY_W, KEY_W) X(12, KEY_E, KEY_E) X(13, KEY_R, KEY_R) X(14, KEY_T, KEY_T) X(15, KEY_Y, KEY_Y) X(16, KEY_U, KEY_U) X(17, KEY_I, KEY_I) X(18, KEY_O, KEY_O) X(19, KEY_P, KEY_P) \
X(20, KEY_A, KEY_A) X(21, KEY_S, KEY_S) X(22, KEY_D, KEY_D) X(23, KEY_F, KEY_F) X(24, KEY_G, KEY_G) X(25, KEY_H, KEY_H) X(26, KEY_J, KEY_J) X(27, KEY_K, KEY_K) X(28, KEY_L, KEY_L) \
X(29, KEY_Z, KEY_Z) X(30, KEY_X, KEY_X) X(31, KEY_C, KEY_C) X(32, KEY_V, KEY_V) X(33, KEY_B, KEY_B) X(34, KEY_N, KEY_N) X(35, KEY_M, KEY_M) \
X(36, KEY_SPACE, KEY_SPACE)

struct {
    KeyboardKey keys[
        #define X(...) +1
        KEYBOARD_KEYS
        #undef X
    ];
    int shiftKeys[
        #define X(...) +1
        KEYBOARD_KEYS
        #undef X
    ];
    int frames[
        #define X(...) +1
        KEYBOARD_KEYS
        #undef X
    ];
    int rows[5];
    int initialFrames;
} qwertyLayout = {
    .rows = { 10, 10, 9, 7, 1 },
    .keys = {
        #define X(i, key, shift) key,
        KEYBOARD_KEYS
        #undef X
    },
    .shiftKeys = {
        #define X(i, key, shift) shift,
        KEYBOARD_KEYS
        #undef X
    },
    .frames = { },
    .initialFrames = 10
};

int getKeysIndex(KeyboardKey key) {
    switch (key) {
        #define X(i, name, shift) case name: return i;
        KEYBOARD_KEYS
        #undef X
        default: return -1;
    }
}
int getKeyShift(KeyboardKey key) {
    int i = getKeysIndex(key);
    if (i == -1) return -1;
    return qwertyLayout.shiftKeys[i];
}
int getKeyNoShift(int c) {
    if (getKeysIndex(c) != -1) return c;
    switch (c) {
        #define X(i, key, shift) case shift: return key;
        KEYBOARD_KEYS
        #undef X
        default: break;
    }
    return -1;
}

void keyResetFrames(KeyboardKey key) {
    int i = getKeysIndex(getKeyNoShift(key));
    if (i == -1) return;
    qwertyLayout.frames[i] = qwertyLayout.initialFrames;
}

// Figure out wether the caps lock is active or not, using GLFW.
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
    if (getKeysIndex(c) == -1) return -1;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        c = getKeyShift(c);
    } else if (!isCapsLockActive()) {
        c = tolower(c);
    }
    return c;
}
