#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    char line[256], *p[MAXARG], ch;
    int lines = 0, linen, ps = 0, pn, i, j;
    for (i = 0; i < argc - 1; i++) {
        p[ps++] = line + lines;
        for (j = 0; j < strlen(argv[i + 1]); j++)
            line[lines++] = argv[i + 1][j];
        line[lines++] = '\0';
    }
    linen = lines; pn = ps; p[pn++] = line + linen;
    while (read(0, &ch, 1) > 0) {
        if (ch == '\n') {
            line[linen++] = '\0'; p[pn++] = 0;
            if (fork() == 0) exec(argv[1], p);
            else {
                wait(0); linen = lines; pn = ps; p[pn++] = line + linen;
            }
        } else if (ch == ' ') {
            line[linen++] = '\0'; p[pn++] = line + linen;
        } else line[linen++] = ch;
    }
    exit(0);
}

