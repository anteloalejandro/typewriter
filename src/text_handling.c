#include <stdlib.h>
#include <string.h>
#include "../headers/shared.h"

#include "../raylib/include/raylib.h"

typedef struct {
    char *str;
    int length;
} Str;

enum Orientation {VERTICAL, HORIZONTAL};

void Str_append(Str *line, char *str, int length) {
    line->str = realloc(line->str, line->length + length);
    memcpy(&line->str[line->length], str, length);
    line->length += length;
}

void Str_destroy(Str *line) {
    free(line->str);
}

// works for either height and width
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
    UnloadFont(data.font);
    data.fontSize = fontSize;
    data.font = LoadFontEx(filename, data.fontSize, 0, 250);
    // spacing = fontSize/defaultFontSize;
    data.spacing = 0;
    data.charWidth = MeasureChar(data.font, 'M', data.fontSize, data.spacing);
    data.lineSpacing = 0.5*data.fontSize; // 0.5 of height added to the linejump
}

