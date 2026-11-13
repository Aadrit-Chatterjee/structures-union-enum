#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define CMD_LEN 16    /* max command line size (incl. '\0') */
#define DESC_LEN 50   /* max description size (incl. '\0') */

typedef enum { CMD_ADD, CMD_DELETE, CMD_VIEW, CMD_UPDATE, CMD_COMPLETE,
               CMD_FIND, CMD_HELP, CMD_EXIT, CMD_UNKNOWN } CommandType;
typedef enum { PENDING, DONE } Status;
typedef struct { int id; char desc[DESC_LEN]; Status status; } Task;

/* Names are in the same order as the enum, so position i == enum value i */
const char *names[] = {"ADD", "DELETE", "VIEW", "UPDATE", "COMPLETE", "FIND", "HELP", "EXIT"};

Task *t;               /* dynamic array of tasks */
int n = 0, cap = 2;    /* count and capacity (starts small so growth is easy to test) */
int eof = 0;           /* becomes 1 when input ends */

/* Safe line reader. Returns 1 if OK, 0 if EOF (eof=1) or line too long */
int readLine(const char *msg, char *buf, int size) {
    printf("%s", msg);
    if (fgets(buf, size, stdin) == NULL) { eof = 1; return 0; }
    char *nl = strchr(buf, '\n');
    if (nl) { *nl = '\0'; return 1; }          /* whole line fit */
    if (feof(stdin)) return 1;                 /* last line had no newline */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);   /* throw away the rest */
    printf("Input too long (max %d characters).\n", size - 1);
    return 0;
}

/* Case-insensitive: convert to upper case, then compare with the names table */
CommandType get_command_type(const char *s) {
    char up[CMD_LEN];
    int i;
    for (i = 0; s[i]; i++) up[i] = toupper((unsigned char)s[i]);
    up[i] = '\0';
    for (i = 0; i < CMD_UNKNOWN; i++)
        if (strcmp(up, names[i]) == 0) return (CommandType)i;
    return CMD_UNKNOWN;
}

/* Read a positive whole-number ID; returns -1 if invalid */
int readId(const char *msg) {
    char b[32], *end;
    if (!readLine(msg, b, sizeof b)) return -1;
    long v = strtol(b, &end, 10);
    if (end == b || *end != '\0' || v <= 0 || v > INT_MAX) {
        printf("Invalid ID (enter a positive whole number).\n");
        return -1;
    }
    return (int)v;
}

int find(int id) {
    for (int i = 0; i < n; i++)
        if (t[i].id == id) return i;
    return -1;
}

/* Ask for an ID and return its index, or -1 (message already printed) */
int pick(void) {
    int id = readId("Task ID: ");
    if (id < 0) return -1;
    int i = find(id);
    if (i < 0) printf("Task %d not found.\n", id);
    return i;
}

void show(const Task *p) {
    printf("ID %d | %s | %s\n", p->id, p->desc, p->status == DONE ? "DONE" : "PENDING");
}

/* Read a non-empty description into d; returns 1 on success */
int readDesc(char *d) {
    if (!readLine("Description: ", d, DESC_LEN)) return 0;
    if (d[0] == '\0') { printf("Description cannot be empty.\n"); return 0; }
    return 1;
}

void add_task(void) {
    int id = readId("Task ID: ");
    if (id < 0) return;
    if (find(id) >= 0) { printf("Task %d already exists.\n", id); return; }
    char d[DESC_LEN];
    if (!readDesc(d)) return;
    if (n == cap) {                                        /* full -> double the size */
        Task *p = realloc(t, cap * 2 * sizeof(Task));
        if (p == NULL) { printf("Out of memory. Task not added.\n"); return; }
        t = p;                                             /* old array stays valid if realloc fails */
        cap *= 2;
    }
    t[n].id = id;
    strcpy(t[n].desc, d);
    t[n].status = PENDING;
    n++;
    printf("Task %d added.\n", id);
}

void delete_task(void) {
    int i = pick();
    if (i < 0) return;
    int id = t[i].id;
    for (; i < n - 1; i++) t[i] = t[i + 1];                /* close the gap */
    n--;
    printf("Task %d deleted.\n", id);
}

void update_task(void) {
    int i = pick();
    char d[DESC_LEN];
    if (i < 0 || !readDesc(d)) return;
    strcpy(t[i].desc, d);
    printf("Task %d updated.\n", t[i].id);
}

void complete_task(void) {
    int i = pick();
    if (i < 0) return;
    t[i].status = DONE;
    printf("Task %d marked DONE.\n", t[i].id);
}

void find_task(void) {
    int i = pick();
    if (i >= 0) show(&t[i]);
}

void view_tasks(void) {
    if (n == 0) { printf("No tasks.\n"); return; }
    for (int i = 0; i < n; i++) show(&t[i]);
}

void help(void) {
    printf("ADD      - add a new task\n"
           "DELETE   - delete a task by ID\n"
           "VIEW     - list all tasks\n"
           "UPDATE   - change a task's description\n"
           "COMPLETE - mark a task as DONE\n"
           "FIND     - show one task by ID\n"
           "HELP     - show this help\n"
           "EXIT     - quit\n");
}

int main() {
    t = malloc(cap * sizeof(Task));
    if (t == NULL) { printf("Out of memory.\n"); return 1; }

    char cmd[CMD_LEN];
    int running = 1;
    while (running && !eof) {
        if (!readLine("\nCommand (type HELP): ", cmd, CMD_LEN)) continue;
        switch (get_command_type(cmd)) {
            case CMD_ADD:      add_task(); break;
            case CMD_DELETE:   delete_task(); break;
            case CMD_VIEW:     view_tasks(); break;
            case CMD_UPDATE:   update_task(); break;
            case CMD_COMPLETE: complete_task(); break;
            case CMD_FIND:     find_task(); break;
            case CMD_HELP:     help(); break;
            case CMD_EXIT:     running = 0; break;
            default:           printf("Unknown command. Type HELP.\n");
        }
    }
    printf("\nExiting program.\n");
    free(t);                      /* runs on EXIT and on EOF */
    return 0;
}