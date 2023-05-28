#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    printf("Hello world!\n");
    malloc(30000);
    printf("done malloc\n");
    exit(0);
}