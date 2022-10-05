#include <string.h>
#include <stdlib.h>


int main(){
    char * a = "abcd";
    char * b = malloc(strlen(a)+1);
    memcpy(b, a, strlen(a)+1);
    printf(b);
}
