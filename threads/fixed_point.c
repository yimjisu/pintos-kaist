#include <stdint.h>

#define f (1 << 14)
int INT_TO_FIXED(int n){
    return(n * f);
}

int FIXED_TO_INT_ZERO(int x){
    return(x / f);
}

int FIXED_TO_INT_NEAR(int x){
    if (x >= 0) {
        return ((x + f/2)/f);
    }

    return ((x - f/2)/f);
}

int ADD_FIXED(int x, int y){
    return(x+y);
}

int SUB_FIXED(int x, int y){
    return(x-y);
}

int MULT_FIXED(int x, int y){
    return ((int64_t)x)*y/f;
}

int DIV_FIXED(int x, int y){
    return((int64_t)x)*f/y;
}

int ADD_FIXED_INT(int x, int n){
    return(x + n *f);
}

int SUB_FIXED_INT(int x, int n){
    return(x - n *f);
}

int MULT_FIXED_INT(int x, int n){
    return(x*n);
}

int DIV_FIXED_INT(int x, int n){
    return(x/n);
}