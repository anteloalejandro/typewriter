#ifndef WORD_WRAP
#define WORD_WRAP

#include "../raylib/include/raylib.h"

void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);   // Draw text using font inside rectangle limits

#endif // WORD_WRAP

