#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int i;
    if (argc < 2) {
        printf("sleep: tick number is not specified\n");
        exit(0);
    }
    i = atoi(argv[1]);
    if (i <= 0) {
        printf("sleep: tick number is invalid\n");
        exit(0);
    }
    sleep(i);
    exit(0);
}
