#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* if compiling for Windows, compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {};

/* if not Windows, include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

// Declare a Lisp Value struct
typedef struct {
  int type;
  long num;
  char* err; // error strings
  char* sym; // symbols
  int count;
  struct lval** cell;  // pointer to list of pointers to lvals
} lval;

// enumeration of possible lval types
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

// enumeration of possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// create pointer to a new number lval
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

// create a pointer to a new error lval
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

// create a pointer to a new symbol lval
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

// create a pointer to a new empty sexpr lval
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}


// print an lval
void lval_print(lval v) {
  switch (v.type) {
    // if number, print it
  case LVAL_NUM: 
    printf("%li", v.num);
    break;

    // if error, check type then print
  case LVAL_ERR:
    if (v.err == LERR_DIV_ZERO) {
      printf("Error: Division by zero!");
    }

    if (v.err == LERR_BAD_OP) {
      printf("Error: Invalid Operator!");
    }

    if (v.err == LERR_BAD_NUM) {
      printf("Error: Invalid number!");
    }
    break;
  }
}

// print an lval followed by a newline
void lval_println(lval v) { lval_print(v); putchar('\n'); }


// Operate on two lvals
lval eval_op(lval x, char* op, lval y) {

  // if either value is an error, return it
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  // otherwise, do math on the number values
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    // check for division by zero
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  return lval_err(LERR_BAD_OP);
}


lval eval(mpc_ast_t* t) {

  // if tagged as number, return it directly
  if (strstr(t->tag, "number")) {
    // check if there's error in conversion
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  // the operator is always the second child
  char* op = t->children[1]->contents;

  // store third child in `x`
  lval x = eval(t->children[2]);

  // iterate remaining children combining
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int arg, char** argv) {

  /* Create parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT, "number : /-?[0-9]+/ ;                        \
                                symbol : '+' | '-' | '*' | '/' ;             \
                                sexpr  : '(' <expr>* ')' ;                   \
                                expr   : <number> | <symbol> | <sexpr> ;     \
                                lispy  : /^/ <expr>* /$/ ;",
	    Number, Symbol, Sexpr, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  /*Never ending loop*/
  while (1) {
    // output prompt and get input
    char* input = readline("lispy> ");
    
    // Add input to history
    add_history(input);
    
    // Attempt to parse the input
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      // evaluate input and print result
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      // print error
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    // free retrieved input
    free(input);
  }

  // undefine and delete parsers
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
  return 0;
}