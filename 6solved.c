#include <stdio.h>
#include <string.h>
#include <stddef.h>

typedef union {
    int i;
    float f;
    char str[20];
} UnionData;

typedef struct {
    int i;
    float f;
    char str[20];
} StructData;

typedef enum { VALUE_INT, VALUE_FLOAT, VALUE_TEXT } ValueType;

typedef struct {
    ValueType type;     /* tag: tells which union member is valid */
    UnionData val;
} TaggedRecord;

/* Setters: each one writes the value AND updates the tag */
void set_int(TaggedRecord *r, int x)     { r->val.i = x; r->type = VALUE_INT; }
void set_float(TaggedRecord *r, float x) { r->val.f = x; r->type = VALUE_FLOAT; }

/* Returns 1 on success, 0 if text does not fit (19 chars + '\0' max) */
int set_text(TaggedRecord *r, const char *s) {
    if (s == NULL || strlen(s) >= sizeof r->val.str) return 0;   /* record unchanged */
    strcpy(r->val.str, s);
    r->type = VALUE_TEXT;
    return 1;
}

/* Prints ONLY the member the tag says is valid */
void show(const TaggedRecord *r) {
    switch (r->type) {
        case VALUE_INT:   printf("int: %d\n", r->val.i); break;
        case VALUE_FLOAT: printf("float: %.2f\n", r->val.f); break;
        case VALUE_TEXT:  printf("text: %s\n", r->val.str); break;
        default:          printf("invalid tag (%d) - value not read\n", (int)r->type);
    }
}

#define INFO(T) printf("%-13s size = %2zu, alignment = %zu\n", #T, sizeof(T), _Alignof(T))

int main() {
    UnionData u;
    StructData s;

    /* 1. Size and alignment */
    printf("--- Size and alignment ---\n");
    INFO(UnionData);
    INFO(StructData);
    INFO(TaggedRecord);

    /* 1 & 3. Addresses: union members share one address, struct members do not */
    printf("\n--- Addresses ---\n");
    printf("u.i   : %p\nu.f   : %p\nu.str : %p\n", (void *)&u.i, (void *)&u.f, (void *)&u.str);
    printf("s.i   : %p\ns.f   : %p\ns.str : %p\n", (void *)&s.i, (void *)&s.f, (void *)&s.str);

    /* 2. Offsets inside the structure */
    printf("\n--- Offsets in StructData ---\n");
    printf("i: %zu, f: %zu, str: %zu\n", offsetof(StructData, i), offsetof(StructData, f),
           offsetof(StructData, str));

    /* 3. Memory for 100 records */
    printf("\n--- Memory for 100 records ---\n");
    printf("StructData: %zu bytes\nUnionData : %zu bytes\nTaggedRecord: %zu bytes\n",
           100 * sizeof(StructData), 100 * sizeof(UnionData), 100 * sizeof(TaggedRecord));

    /* Struct holds all three values at once */
    s.i = 10; s.f = 220.5f; strcpy(s.str, "Struct Programming");
    printf("\n--- Struct (all values coexist) ---\n%d, %.2f, %s\n", s.i, s.f, s.str);

    /* Union: print each member right after it is assigned (only the active one) */
    printf("\n--- Union (one value at a time) ---\n");
    u.i = 10;                       printf("u.i = %d\n", u.i);
    u.f = 220.5f;                   printf("u.f = %.2f\n", u.f);
    strcpy(u.str, "Union Programming"); printf("u.str = %s\n", u.str);

    /* 7. Five mixed tagged records */
    TaggedRecord rec[5];
    set_int(&rec[0], 10);
    set_float(&rec[1], 220.5f);
    set_text(&rec[2], "Union Programming");
    set_int(&rec[3], 42);
    set_text(&rec[4], "Hello");
    printf("\n--- Tagged records ---\n");
    for (int i = 0; i < 5; i++) { printf("Record %d -> ", i + 1); show(&rec[i]); }

    /* Edge cases */
    printf("\n--- Edge cases ---\n");
    printf("19 chars: %s\n", set_text(&rec[0], "ABCDEFGHIJKLMNOPQRS") ? "stored" : "rejected");
    show(&rec[0]);
    printf("20 chars: %s\n", set_text(&rec[1], "ABCDEFGHIJKLMNOPQRST") ? "stored" : "rejected");
    show(&rec[1]);                  /* still the old float, so it is unchanged */

    set_int(&rec[3], 7);    printf("int   -> "); show(&rec[3]);
    set_text(&rec[3], "Hi"); printf("text  -> "); show(&rec[3]);
    set_float(&rec[3], 1.5f); printf("float -> "); show(&rec[3]);

    rec[4].type = (ValueType)99;    /* corrupt the tag on purpose */
    printf("bad tag -> "); show(&rec[4]);

    printf("\n--- Observations ---\n");
    printf("1. Union members start at the same address, so writing one overwrites the others.\n");
    printf("2. Struct members get separate space; padding may be added so each field is aligned.\n");
    printf("3. A union is only as big as its largest member, so it saves memory when a record\n");
    printf("   holds just one of several types. The tag costs a little, but is still smaller\n");
    printf("   than a struct on most platforms. The price: values cannot coexist.\n");
    return 0;
}