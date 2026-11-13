#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------- Container API ----------------
   Every function returns 1 on success and 0 on failure (bad pointer, wrong type,
   allocation failure). A Container that owns a string must NEVER be copied with
   '=' (two owners -> double free). Use container_copy instead.
   Rule: call container_init once on every new Container before using it. */

typedef enum { TYPE_EMPTY, TYPE_INT, TYPE_FLOAT, TYPE_CHAR, TYPE_STRING } DataType;

typedef union {
    int i;
    float f;
    char c;
    char *s;            /* heap copy owned by the container */
} Data;

typedef struct {
    Data data;
    DataType type;      /* tells which member of 'data' is valid */
} Container;

/* Test helper: when set to 1, the next allocation fails (simulates out-of-memory) */
static int fail_next = 0;
static void *xmalloc(size_t n) {
    if (fail_next) { fail_next = 0; return NULL; }
    return malloc(n);
}

/* Make an independent heap copy of a string (our own strdup) */
static char *copy_text(const char *s) {
    size_t n = strlen(s) + 1;                  /* +1 for '\0' */
    char *p = xmalloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

void container_init(Container *c) {
    if (c) c->type = TYPE_EMPTY;
}

/* Frees the string (if any) and makes the container empty. Safe to call repeatedly. */
void container_clear(Container *c) {
    if (c == NULL) return;
    if (c->type == TYPE_STRING) free(c->data.s);
    c->type = TYPE_EMPTY;
}

int container_set_int(Container *c, int v) {
    if (c == NULL) return 0;
    container_clear(c);
    c->data.i = v; c->type = TYPE_INT;
    return 1;
}

int container_set_float(Container *c, float v) {
    if (c == NULL) return 0;
    container_clear(c);
    c->data.f = v; c->type = TYPE_FLOAT;
    return 1;
}

int container_set_char(Container *c, char v) {
    if (c == NULL) return 0;
    container_clear(c);
    c->data.c = v; c->type = TYPE_CHAR;
    return 1;
}

/* Copy FIRST, free old value AFTER. So if allocation fails the old value is kept,
   and passing the container's own string as 's' is also safe. */
int container_set_string(Container *c, const char *s) {
    if (c == NULL || s == NULL) return 0;
    char *p = copy_text(s);
    if (p == NULL) return 0;
    container_clear(c);
    c->data.s = p; c->type = TYPE_STRING;
    return 1;
}

/* Getters: return 1/0 for success; the value goes into *out (so 0 is a valid value) */
int container_get_int(const Container *c, int *out) {
    if (c == NULL || out == NULL || c->type != TYPE_INT) return 0;
    *out = c->data.i;
    return 1;
}

int container_get_float(const Container *c, float *out) {
    if (c == NULL || out == NULL || c->type != TYPE_FLOAT) return 0;
    *out = c->data.f;
    return 1;
}

int container_get_char(const Container *c, char *out) {
    if (c == NULL || out == NULL || c->type != TYPE_CHAR) return 0;
    *out = c->data.c;
    return 1;
}

/* *out points INSIDE the container: do not free it, and it is invalid after clear/set */
int container_get_string(const Container *c, const char **out) {
    if (c == NULL || out == NULL || c->type != TYPE_STRING) return 0;
    *out = c->data.s;
    return 1;
}

/* Deep copy. 'dst' must be initialised. Copying onto itself does nothing (success). */
int container_copy(Container *dst, const Container *src) {
    if (dst == NULL || src == NULL) return 0;
    if (dst == src) return 1;
    if (src->type == TYPE_STRING)
        return container_set_string(dst, src->data.s);   /* new separate buffer */
    container_clear(dst);
    dst->data = src->data;                               /* plain values are safe to copy */
    dst->type = src->type;
    return 1;
}

const char *container_type_name(const Container *c) {
    if (c == NULL) return "NULL";
    switch (c->type) {
        case TYPE_EMPTY:  return "EMPTY";
        case TYPE_INT:    return "INT";
        case TYPE_FLOAT:  return "FLOAT";
        case TYPE_CHAR:   return "CHAR";
        case TYPE_STRING: return "STRING";
    }
    return "INVALID";
}

void container_print(const Container *c) {
    if (c == NULL) { printf("(null)"); return; }
    switch (c->type) {
        case TYPE_EMPTY:  printf("(empty)"); break;
        case TYPE_INT:    printf("%d", c->data.i); break;
        case TYPE_FLOAT:  printf("%.2f", c->data.f); break;
        case TYPE_CHAR:   printf("'%c'", c->data.c); break;
        case TYPE_STRING: printf("\"%s\"", c->data.s); break;
        default:          printf("(invalid type)");
    }
}

/* ---------------- Demonstration ---------------- */

const char *result(int r) { return r ? "success" : "FAILED"; }

void show(const char *label, const Container *c) {
    printf("%s: %-6s ", label, container_type_name(c));
    container_print(c);
    printf("\n");
}

int main() {
    Container a[6], cp, t, e;
    char label[16];
    for (int i = 0; i < 6; i++) container_init(&a[i]);
    container_init(&cp); container_init(&t); container_init(&e);

    container_set_int(&a[0], 10);
    container_set_float(&a[1], 3.14f);
    container_set_char(&a[2], 'A');
    container_set_string(&a[3], "Hello");
    container_set_int(&a[4], 0);
    container_set_string(&a[5], "Data");

    printf("--- All containers ---\n");
    for (int i = 0; i < 6; i++) { sprintf(label, "[%d]", i); show(label, &a[i]); }

    printf("\n--- Getters ---\n");
    int iv = -1; float fv;
    int ok = container_get_int(&a[4], &iv);
    printf("get_int on valid 0   : %s, value = %d\n", result(ok), iv);
    printf("get_float on an INT  : %s\n", result(container_get_float(&a[0], &fv)));
    show("empty container", &e);

    printf("\n--- Empty string ---\n");
    container_set_string(&e, "");
    show("e", &e);

    printf("\n--- Deep copy ---\n");
    container_copy(&cp, &a[5]);
    container_set_string(&a[5], "Changed");
    show("original", &a[5]);
    show("copy    ", &cp);
    printf("self-copy: %s\n", result(container_copy(&a[5], &a[5])));
    show("original", &a[5]);

    printf("\n--- Same source pointer ---\n");
    const char *p;
    container_get_string(&a[3], &p);
    printf("set_string(own pointer): %s\n", result(container_set_string(&a[3], p)));
    show("[3]", &a[3]);

    printf("\n--- Allocation failure ---\n");
    fail_next = 1;
    printf("set_string: %s\n", result(container_set_string(&a[3], "New text")));
    show("[3] kept", &a[3]);
    fail_next = 1;
    printf("copy      : %s\n", result(container_copy(&cp, &a[5])));
    show("cp kept", &cp);

    printf("\n--- Type transitions ---\n");
    container_set_string(&t, "text");  show("STRING", &t);
    container_set_int(&t, 42);         show("INT   ", &t);
    container_set_string(&t, "again"); show("STRING", &t);
    container_set_float(&t, 2.5f);     show("FLOAT ", &t);

    printf("\n--- Cleanup ---\n");
    for (int i = 0; i < 6; i++) { container_clear(&a[i]); container_clear(&a[i]); }
    container_clear(&cp); container_clear(&t); container_clear(&e);
    show("[3] after clear", &a[3]);
    return 0;
}