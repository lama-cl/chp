int main(){
// mixed-direct-alias.c
void *pv;
int *pi = pv;
char *pc = pv;
double *pd = (double *)pc;
}

// expected: pv : void * => ⟨| int *, char * => double * |⟩