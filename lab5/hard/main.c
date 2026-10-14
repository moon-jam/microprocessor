extern unsigned int evaluate(unsigned int a, char op1,
unsigned int b, char op2, unsigned int c, char op3, unsigned
int d);
int main(){
	volatile unsigned int ans = evaluate(12, '+', 34, '*', 2, '*', 3);
    while(1) ;
    return 0;
}
