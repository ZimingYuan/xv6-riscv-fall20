#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

const char *now = ".", *par = "..";

char* fmtname(char *path) {
    char *p;
    for(p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;
    return p;
}

void find(char *path, char *pattern) {
    char buf[128], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        printf("find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        printf("find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:
            if (strcmp(pattern, fmtname(path)) == 0) printf("%s\n", path);
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0) continue;
                if (strcmp(de.name, now) == 0 || strcmp(de.name, par) == 0) continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, pattern);
            }
            break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("find: argument is less than 2\n");
        exit(0);
    }
    find(argv[1], argv[2]); exit(0);
}
