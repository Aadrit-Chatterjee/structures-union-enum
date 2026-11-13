#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#define MAX 1000      /* upper bound on number of rectangles */
#define EPS 1e-4f     /* tolerance: |width - height| < EPS means "square" */

typedef struct {
    int id;
    float width, height, area, perimeter;
} Rectangle;

Rectangle *r;         /* dynamic array */
int n = 0, cap = 0;   /* n = rectangles stored, cap = space allocated */

/* Read an int in [lo, hi]; keep asking until valid */
int readInt(const char *msg, int lo, int hi) {
    int x;
    while (1) {
        printf("%s", msg);
        if (scanf("%d", &x) == 1 && x >= lo && x <= hi) return x;
        if (feof(stdin)) exit(1);
        printf("Invalid! Enter a number from %d to %d.\n", lo, hi);
        while (getchar() != '\n' && !feof(stdin));   /* throw away bad input */
    }
}

/* Read a finite positive float (rejects 0, negatives, nan, inf) */
float readPos(const char *msg) {
    float x;
    while (1) {
        printf("%s", msg);
        if (scanf("%f", &x) == 1 && isfinite(x) && x > 0) return x;
        if (feof(stdin)) exit(1);
        printf("Invalid! Enter a finite positive number.\n");
        while (getchar() != '\n' && !feof(stdin));
    }
}

/* Recompute all derived fields */
void calculate(Rectangle *p) {
    p->area = p->width * p->height;
    p->perimeter = 2 * (p->width + p->height);
}

/* Return index of rectangle with this id, or -1 */
int find(int id) {
    for (int i = 0; i < n; i++)
        if (r[i].id == id) return i;
    return -1;
}

void show(Rectangle *p) {
    printf("ID %d: W=%.2f H=%.2f Area=%.2f Perimeter=%.2f (%s)\n", p->id, p->width,
           p->height, p->area, p->perimeter,
           fabsf(p->width - p->height) < EPS ? "Square" : "Non-square");
}

void display(void) {
    if (n == 0) { printf("No rectangles.\n"); return; }
    for (int i = 0; i < n; i++) show(&r[i]);
}

void add(void) {
    if (n == MAX) { printf("Full (max %d).\n", MAX); return; }
    if (n == cap) {                                   /* array full -> grow */
        int newCap = cap * 2 > MAX ? MAX : cap * 2;
        Rectangle *t = realloc(r, newCap * sizeof(Rectangle));
        if (t == NULL) { printf("Resize failed! Nothing added.\n"); return; }
        r = t;                                        /* old data stays valid on failure */
        cap = newCap;
    }
    int id = readInt("ID: ", 1, INT_MAX);
    if (find(id) != -1) { printf("Duplicate ID!\n"); return; }
    r[n].id = id;
    r[n].width = readPos("Width: ");
    r[n].height = readPos("Height: ");
    calculate(&r[n]);
    n++;
}

void search(void) {
    int i = find(readInt("ID to search: ", 1, INT_MAX));
    if (i < 0) printf("Not found.\n"); else show(&r[i]);
}

void update(void) {
    int i = find(readInt("ID to update: ", 1, INT_MAX));
    if (i < 0) { printf("Not found.\n"); return; }
    r[i].width = readPos("New width: ");
    r[i].height = readPos("New height: ");
    calculate(&r[i]);                                 /* keep area/perimeter consistent */
    show(&r[i]);
}

void delete_rect(void) {
    int i = find(readInt("ID to delete: ", 1, INT_MAX));
    if (i < 0) { printf("Not found.\n"); return; }
    for (; i < n - 1; i++) r[i] = r[i + 1];           /* shift left to fill the gap */
    n--;
}

void stats(void) {
    if (n == 0) { printf("No rectangles.\n"); return; }
    float total = 0;
    int big = 0, small = 0;
    for (int i = 0; i < n; i++) {
        total += r[i].area;
        if (r[i].area > r[big].area) big = i;
        if (r[i].area < r[small].area) small = i;
    }
    printf("Largest:  "); show(&r[big]);
    printf("Smallest: "); show(&r[small]);
    printf("Total area = %.2f, Average area = %.2f\n", total, total / n);
}

/* Compare for qsort: by area, then by ID */
int cmp(const void *a, const void *b) {
    const Rectangle *x = a, *y = b;
    if (x->area != y->area) return x->area < y->area ? -1 : 1;
    return x->id - y->id;
}

void sorted(void) {
    qsort(r, n, sizeof(Rectangle), cmp);
    display();
}

void filter(void) {
    float t = readPos("Area threshold: ");
    int found = 0;
    for (int i = 0; i < n; i++)
        if (r[i].area > t) { show(&r[i]); found = 1; }
    if (!found) printf("None above %.2f.\n", t);
}

int main() {
    int k = readInt("How many rectangles (1-1000)? ", 1, MAX);
    r = malloc(k * sizeof(Rectangle));
    if (r == NULL) { printf("Allocation failed.\n"); return 1; }
    cap = k;
    while (n < k) add();                              /* duplicates are retried */

    int c;
    do {
        printf("\n1.Add 2.Display 3.Search 4.Update 5.Delete 6.Stats 7.Sorted 8.Filter 0.Exit\n");
        c = readInt("Choice: ", 0, 8);
        switch (c) {
            case 1: add(); break;
            case 2: display(); break;
            case 3: search(); break;
            case 4: update(); break;
            case 5: delete_rect(); break;
            case 6: stats(); break;
            case 7: sorted(); break;
            case 8: filter(); break;
        }
    } while (c != 0);

    free(r);
    return 0;
}