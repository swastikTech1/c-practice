/*
 * Full Calculator - Basic to Scientific
 * Build: gcc Calcultor.c -o Calcultor -lm
 * Operations: + - * / % ^ sqrt sin cos tan asin acos atan sinh cosh tanh log ln exp abs fact
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14159265358979323846
#define E  2.71828182845904523536
#define MAX_OP 16

static double fact(double n)
{
    if (n < 0 || n != floor(n))
        return -1;  /* invalid */
    if (n <= 1)
        return 1;
    double r = 1;
    for (long i = 2; i <= (long)n; i++)
        r *= i;
    return r;
}

static void to_lower(char *s)
{
    for (; *s; s++)
        if (*s >= 'A' && *s <= 'Z')
            *s += 32;
}

static int is_unary(const char *op)
{
    return strcmp(op, "sqrt") == 0 || strcmp(op, "sin") == 0 || strcmp(op, "cos") == 0 ||
           strcmp(op, "tan") == 0 || strcmp(op, "asin") == 0 || strcmp(op, "acos") == 0 ||
           strcmp(op, "atan") == 0 || strcmp(op, "sinh") == 0 || strcmp(op, "cosh") == 0 ||
           strcmp(op, "tanh") == 0 || strcmp(op, "log") == 0 || strcmp(op, "ln") == 0 ||
           strcmp(op, "exp") == 0 || strcmp(op, "abs") == 0 || strcmp(op, "fact") == 0 ||
           strcmp(op, "floor") == 0 || strcmp(op, "ceil") == 0 || strcmp(op, "inv") == 0 ||
           strcmp(op, "neg") == 0 || strcmp(op, "pi") == 0 || strcmp(op, "e") == 0;
}

static int compute(double a, double b, const char *op, double *result)
{
    char opbuf[MAX_OP];
    strncpy(opbuf, op, MAX_OP - 1);
    opbuf[MAX_OP - 1] = '\0';
    to_lower(opbuf);

    if (strcmp(opbuf, "+") == 0)        { *result = a + b; return 0; }
    if (strcmp(opbuf, "-") == 0)        { *result = a - b; return 0; }
    if (strcmp(opbuf, "*") == 0)        { *result = a * b; return 0; }
    if (strcmp(opbuf, "/") == 0)        { if (b == 0) return -1; *result = a / b; return 0; }
    if (strcmp(opbuf, "%") == 0)        { if ((long)b == 0) return -1; *result = (double)((long)a % (long)b); return 0; }
    if (strcmp(opbuf, "^") == 0)        { *result = pow(a, b); return 0; }
    if (strcmp(opbuf, "pow") == 0)      { *result = pow(a, b); return 0; }
    if (strcmp(opbuf, "p") == 0)        { *result = (a / 100.0) * b; return 0; }
    if (strcmp(opbuf, "//") == 0)       { if ((long)b == 0) return -1; *result = floor(a / b); return 0; }

    /* Unary operations (use 'a', ignore b) */
    if (strcmp(opbuf, "sqrt") == 0)     { if (a < 0) return -2; *result = sqrt(a); return 0; }
    if (strcmp(opbuf, "sin") == 0)      { *result = sin(a); return 0; }
    if (strcmp(opbuf, "cos") == 0)      { *result = cos(a); return 0; }
    if (strcmp(opbuf, "tan") == 0)      { *result = tan(a); return 0; }
    if (strcmp(opbuf, "asin") == 0)     { if (a < -1 || a > 1) return -2; *result = asin(a); return 0; }
    if (strcmp(opbuf, "acos") == 0)     { if (a < -1 || a > 1) return -2; *result = acos(a); return 0; }
    if (strcmp(opbuf, "atan") == 0)     { *result = atan(a); return 0; }
    if (strcmp(opbuf, "sinh") == 0)     { *result = sinh(a); return 0; }
    if (strcmp(opbuf, "cosh") == 0)     { *result = cosh(a); return 0; }
    if (strcmp(opbuf, "tanh") == 0)     { *result = tanh(a); return 0; }
    if (strcmp(opbuf, "log") == 0)      { if (a <= 0) return -2; *result = log10(a); return 0; }
    if (strcmp(opbuf, "ln") == 0)       { if (a <= 0) return -2; *result = log(a); return 0; }
    if (strcmp(opbuf, "exp") == 0)      { *result = exp(a); return 0; }
    if (strcmp(opbuf, "abs") == 0)      { *result = fabs(a); return 0; }
    if (strcmp(opbuf, "fact") == 0)     { *result = fact(a); return (*result < 0) ? -2 : 0; }
    if (strcmp(opbuf, "floor") == 0)    { *result = floor(a); return 0; }
    if (strcmp(opbuf, "ceil") == 0)     { *result = ceil(a); return 0; }
    if (strcmp(opbuf, "inv") == 0)      { if (a == 0) return -1; *result = 1.0 / a; return 0; }
    if (strcmp(opbuf, "neg") == 0)      { *result = -a; return 0; }
    if (strcmp(opbuf, "pi") == 0)       { *result = PI; return 0; }
    if (strcmp(opbuf, "e") == 0)        { *result = E; return 0; }

    return 1;  /* unknown */
}

int main(void)
{
    char op[MAX_OP];
    double a, b, result;

    printf("=== Calculator (Basic + Scientific) ===\n\n");
    printf("Basic:     + - * / %% ^ p(percent) //(quotient)\n");
    printf("Scientific: sqrt sin cos tan asin acos atan sinh cosh tanh\n");
    printf("            log ln exp abs fact floor ceil inv neg pi e\n");
    printf("Format: number operator number  (unary: number op 0)\n");
    printf("Quit: 0 quit 0\n\n");

    for (;;)
    {
        printf("> ");
        if (scanf("%lf %15s %lf", &a, op, &b) != 3)
            break;

        if (strcmp(op, "quit") == 0 || strcmp(op, "q") == 0)
            break;

        int err = compute(a, b, op, &result);
        if (err == 0)
            printf("  => %.10g\n\n", result);
        else if (err == -1)
            printf("  => Error: Division by zero.\n\n");
        else if (err == -2)
            printf("  => Error: Invalid input (domain error).\n\n");
        else
            printf("  => Error: Unknown operator '%s'.\n\n", op);
    }

    printf("Done.\n");
    return 0;
}
