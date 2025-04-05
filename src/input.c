#include "../raylib/include/raygui.h"
#include <ctype.h>
#include <GLFW/glfw3.h>

// STATIC VARIABLES
#define KEYBOARD_KEYS \
X(0, KEY_ONE) X(1, KEY_TWO) X(2, KEY_THREE) X(3, KEY_FOUR) X(4, KEY_FIVE) X(5, KEY_SIX) X(6, KEY_SEVEN) X(7, KEY_EIGHT) X(8, KEY_NINE) X(9, KEY_ZERO) \
X(10, KEY_Q) X(11, KEY_W) X(12, KEY_E) X(13, KEY_R) X(14, KEY_T) X(15, KEY_Y) X(16, KEY_U) X(17, KEY_I) X(18, KEY_O) X(19, KEY_P) \
X(20, KEY_A) X(21, KEY_S) X(22, KEY_D) X(23, KEY_F) X(24, KEY_G) X(25, KEY_H) X(26, KEY_J) X(27, KEY_K) X(28, KEY_L) \
X(29, KEY_Z) X(30, KEY_X) X(31, KEY_C) X(32, KEY_V) X(33, KEY_B) X(34, KEY_N) X(35, KEY_M)

struct {
    KeyboardKey keys[
        #define X(i, key) +1
        KEYBOARD_KEYS
        #undef X
    ];
    int frames[
        #define X(i, key) +1
        KEYBOARD_KEYS
        #undef X
    ];
    int rows[4];
    int initialFrames;
} keyboardKeys = {
    .rows = { 10, 10, 9, 7 },
    .keys = {
        #define X(i, key) key,
        KEYBOARD_KEYS
        #undef X
    },
    .frames = { },
    .initialFrames = 10
};

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
    int key = GetKeyPressed();
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        switch (key) {
            case KEY_SEMICOLON: key = ':'; break;
            case KEY_SLASH: key = '?'; break;
            case KEY_COMMA: key = '<'; break;
            case KEY_PERIOD: key = '>'; break;
            default: break;
        }

    } else if (!isCapsLockActive()) {
        key = tolower(key);
    }
    return key;
}
