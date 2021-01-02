#include "kernel/types.h"
#include "user/user.h"

void f(int pp) {
    int prime;
    if (read(pp, (char *)&prime, 4) != 0) {
        printf("prime %d\n", prime);
        int p[2]; pipe(p);
        if (fork() == 0) {
            close(p[1]); f(p[0]);
        } else {
            int t;
            while (read(pp, (char *)&t, 4) != 0) {
                if (t % prime != 0) write(p[1], (char *)&t, 4);
            }
            close(p[1]); close(p[0]); close(pp); wait(0);
        }
    } else close(pp);
}
int main() {
    int i, p[2]; pipe(p);
    if (fork() == 0) {
        close(p[1]); f(p[0]);
    } else {
        for (i = 2; i < 36; i++) write(p[1], (char *)&i, 4);
        close(p[1]); close(p[0]); wait(0);
    }
    exit(0);
}
