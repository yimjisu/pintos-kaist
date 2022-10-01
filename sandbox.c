#include <stdint.h>
#include <stdio.h>

struct s {
    uintptr_t r;
};

int main(){
    struct s s1;
    int n = 3;
    s1.r = &n;
    printf("%d\n", s1.r % 3);
}

