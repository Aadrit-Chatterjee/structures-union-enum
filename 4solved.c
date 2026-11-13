/* 4.c - Student Records and Academic Analytics System
 * Design notes:
 *  - Records live in a dynamic array (L.data) with separate count and capacity.
 *  - Sorting works on a COPY, so the original insertion order is preserved.
 *  - Capacity doubles via realloc; on failure the original pointer is kept.
 *  - Input is read as whole lines (fgets) and parsed strictly (strtol/strtod).
 *  - Memory is freed on every normal exit path through atexit(cleanup). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define NAME_LEN 50          /* 49 characters + '\0' */
#define MAX_CAP 1000000      /* upper limit for the initial capacity */

typedef struct { int roll; char name[NAME_LEN]; int age; float gpa; } Student;
static struct { Student *data; int count, cap; } L;

static void cleanup(void) { free(L.data); L.data = NULL; }

/* ---------- input helpers ---------- */
/* Returns 1 on a complete line, 0 if the line was too long for buf (rest discarded). */
static int read_line(const char *prompt, char *buf, size_t n) {
    printf("%s", prompt);
    if (!fgets(buf, (int)n, stdin)) { puts("\nInput closed. Exiting."); exit(0); }
    size_t len = strlen(buf);
    if (len && buf[len - 1] == '\n') { buf[len - 1] = 0; return 1; }
    if (feof(stdin)) return 1;
    int c; while ((c = getchar()) != '\n' && c != EOF);
    return 0;
}

/* Re-prompts until a number in [lo, hi] is entered (whole number if integer). */
static double get_num(const char *prompt, double lo, double hi, int integer) {
    char buf[64], *end;
    for (;;) {
        if (read_line(prompt, buf, sizeof buf)) {
            double v = integer ? (double)strtol(buf, &end, 10) : strtod(buf, &end);
            if (end != buf && !*end && v >= lo && v <= hi) return v;
        }
        printf("  Error: enter a %s between %.10g and %.10g.\n",
               integer ? "whole number" : "number", lo, hi);
    }
}

static int ask_roll(const char *prompt) { return (int)get_num(prompt, 1, INT_MAX, 1); }

static void get_name(char *out) {
    char buf[256];
    for (;;) {
        if (read_line("  Name: ", buf, sizeof buf) && buf[0] && strlen(buf) < NAME_LEN) {
            strcpy(out, buf);
            return;
        }
        printf("  Error: name must be 1-%d characters.\n", NAME_LEN - 1);
    }
}

static void read_details(Student *s) {
    get_name(s->name);
    s->age = (int)get_num("  Age : ", 16, 100, 1);
    s->gpa = (float)get_num("  GPA : ", 0, 4, 0);
}

/* ---------- allocation ---------- */
/* Grows/shrinks L.data to newcap records; leaves everything untouched on failure. */
static int resize(int newcap) {
    if (newcap <= 0 || (size_t)newcap > SIZE_MAX / sizeof(Student)) return 0;
    Student *p = realloc(L.data, (size_t)newcap * sizeof(Student));
    if (!p) return 0;
    L.data = p; L.cap = newcap;
    return 1;
}

/* ---------- record management ---------- */
static int find(int roll) {
    for (int i = 0; i < L.count; i++) if (L.data[i].roll == roll) return i;
    return -1;
}

static void print_rows(const Student *s, int n) {
    printf("%-8s %-50s %4s %6s\n", "Roll", "Name", "Age", "GPA");
    for (int i = 0; i < n; i++)
        printf("%-8d %-50s %4d %6.2f\n", s[i].roll, s[i].name, s[i].age, s[i].gpa);
}

static void add_student(void) {
    Student s;
    s.roll = ask_roll("  Roll number (positive): ");
    if (find(s.roll) >= 0) { printf("Error: roll number %d already exists.\n", s.roll); return; }
    read_details(&s);
    if (L.count == L.cap) {
        int nc = L.cap > INT_MAX / 2 ? INT_MAX : L.cap * 2;
        if (nc == L.cap || !resize(nc)) { puts("Error: cannot grow storage; record not added."); return; }
        printf("Capacity grown to %d.\n", L.cap);
    }
    L.data[L.count++] = s;
    puts("Success: student added.");
}

static void display_students(void) {
    if (!L.count) puts("No records to display (collection is empty).");
    else print_rows(L.data, L.count);
}

static void search_student(void) {
    int roll = ask_roll("Roll number to search: "), i = find(roll);
    if (i < 0) printf("Error: no student with roll number %d.\n", roll);
    else print_rows(&L.data[i], 1);
}

static void update_student(void) {
    int roll = ask_roll("Roll number to update: "), i = find(roll);
    if (i < 0) { printf("Error: no student with roll number %d.\n", roll); return; }
    read_details(&L.data[i]);
    puts("Success: record updated.");
}

static void delete_student(void) {
    int roll = ask_roll("Roll number to delete: "), i = find(roll);
    if (i < 0) { printf("Error: no student with roll number %d.\n", roll); return; }
    memmove(&L.data[i], &L.data[i + 1], (size_t)(L.count - i - 1) * sizeof(Student));
    L.count--;
    puts("Success: record deleted.");
}

/* analytics */
static void summary(void) {
    if (!L.count) { puts("No summary available: collection is empty."); return; }
    int hi = 0, lo = 0;
    double sum = 0;
    for (int i = 0; i < L.count; i++) {
        if (L.data[i].gpa > L.data[hi].gpa) hi = i;
        if (L.data[i].gpa < L.data[lo].gpa) lo = i;
        sum += L.data[i].gpa;
    }
    printf("Highest GPA: %s (Roll %d) - %.2f\n", L.data[hi].name, L.data[hi].roll, L.data[hi].gpa);
    printf("Lowest GPA : %s (Roll %d) - %.2f\n", L.data[lo].name, L.data[lo].roll, L.data[lo].gpa);
    printf("Average GPA: %.2f over %d student(s)\n", sum / L.count, L.count);
}

/* GPA descending, ties broken by roll number ascending. */
static int cmp(const void *a, const void *b) {
    const Student *x = a, *y = b;
    if (x->gpa != y->gpa) return x->gpa < y->gpa ? 1 : -1;
    return (x->roll > y->roll) - (x->roll < y->roll);
}

static void show_sorted(void) {
    if (!L.count) { puts("Nothing to sort (collection is empty)."); return; }
    Student *c = malloc((size_t)L.count * sizeof *c);
    if (!c) { puts("Error: not enough memory to sort."); return; }
    memcpy(c, L.data, (size_t)L.count * sizeof *c);
    qsort(c, (size_t)L.count, sizeof *c, cmp);
    print_rows(c, L.count);
    free(c);
}

int main(void) {
    atexit(cleanup);
    puts("=== Student Records & Academic Analytics ===");
    for (;;) {
        int n = (int)get_num("Initial capacity: ", 1, MAX_CAP, 1);
        if (resize(n)) break;
        puts("  Error: could not allocate that much memory. Try a smaller value.");
    }
    for (;;) {
        puts("\n1 Add | 2 Display | 3 Search | 4 Update | 5 Delete | 6 GPA summary | 7 Sorted by GPA | 0 Exit");
        switch ((int)get_num("Choice: ", 0, 7, 1)) {
            case 1: add_student(); break;
            case 2: display_students(); break;
            case 3: search_student(); break;
            case 4: update_student(); break;
            case 5: delete_student(); break;
            case 6: summary(); break;
            case 7: show_sorted(); break;
            case 0: puts("Goodbye."); return 0;
        }
    }
}