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

int main(int arg, char** argv) {

  /* Create parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT, "number : /-?[0-9]+/ ;                          \
                                operator : '+' | '-' | '*' | '/' ;             \
                                expr : <number> | '(' <operator> <expr>+ ')' ; \
                                lispy : /^/ <operator> <expr>+ /$/ ;",
	    Number, Operator, Expr, Lispy);

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
      // print AST on success
      mpc_ast_print(r.output);
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
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  return 0;
}