#include <stdlib.h>
#include <stdio.h>
#include "../headers/shared.h"

typedef struct {
    char c;
    int permanence;
    int visibility;
    Position position;
} FloatingChar;
typedef struct FloatingCharNode {
    FloatingChar *fchar;
    struct FloatingCharNode *next;
} FloatingCharNode;
typedef struct {
    FloatingCharNode *head;
    int length;
} FloatingCharList;

void FloatingChar_print(FloatingCharList *list) {
    FloatingCharNode *node = list->head;
    while (node != NULL) {
        FloatingChar m = *node->fchar;
        printf("{ %c, %d, %d, (%d,%d) } -> ", m.c, m.permanence, m.visibility, m.position.x, m.position.y);
        node = node->next;
    }
    printf("NULL\n");
}

void FloatingChar_destroy(FloatingCharNode *node) {
    free(node->fchar);
    free(node);
}

void FloatingCharList_insert(FloatingCharList *list, char c, Position position) {
    FloatingChar *fchar = malloc(sizeof(FloatingChar));
    FloatingCharNode *node = malloc(sizeof(FloatingCharNode));

    fchar->c = c;
    fchar->position = position;
    fchar->permanence = GetRandomValue(2, 5);
    fchar->visibility = fchar->permanence;

    node->fchar = fchar;
    node->next = list->head;
    list->head = node;

    list->length++;

}

void FloatingCharList_eraseAll(FloatingCharList *list, Position position) {
    if (list->head == NULL) return;
    FloatingCharNode *prev = NULL, *node = list->head;
    while (node != NULL) {
        FloatingChar *m = node->fchar;
        if (m->position.x == position.x && m->position.y == position.y) {
            if (m->visibility <= 1) {
                FloatingCharNode *tmp = node;
                node = node->next;
                if (prev == NULL) list->head = node;
                else prev->next = node;
                FloatingChar_destroy(tmp);
            } else {
                m->visibility--;
                prev = node;
                node = node->next;
            }
        } else {
            prev = node;
            node = node->next;
        }
    }
}

void FloatingCharList_destroy(FloatingCharList *list) {
    FloatingCharNode *node = list->head;
    while (node != NULL) {
        FloatingCharNode *tmp = node;
        node = node->next;
        FloatingChar_destroy(tmp);
    }
}
