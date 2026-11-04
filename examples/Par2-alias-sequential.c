int main(){
// alias-sequential.c
void *pv;
int *pi = pv;
char *pc = (char *)pi;
}

// expected: pv : void * => int * => char *