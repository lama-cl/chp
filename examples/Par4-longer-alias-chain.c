// longer-alias-chain.c
int main() {
void *pv;
int *pi = pv;
char *pc = (char *)pi;
double *pd = (double *)pc;
}
// expected: pv : void * => int * => char * => double *