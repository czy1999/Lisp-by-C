#include "mpc.h"


#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';
	return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* ö�ٴ������� */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* ö�ٿ��ܵ�ֵ���� */
enum { LVAL_NUM, LVAL_ERR };

/* �����ṹ������ */
typedef struct {
	int type;
	long num;
	int err;
} lval;

/* Create a new number type lval */
lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

/* ��ӡ����� */
void lval_print(lval v) {
	switch (v.type) {
		/*���û���ӡ���*/
	case LVAL_NUM: printf("%li", v.num); break;

		/* ���������� */
	case LVAL_ERR:
		/* �������һ�ִ��� */
		if (v.err == LERR_DIV_ZERO) {
			printf("Error: Division By Zero!");
		}
		if (v.err == LERR_BAD_OP) {
			printf("Error: Invalid Operator!");
		}
		if (v.err == LERR_BAD_NUM) {
			printf("Error: Invalid Number!");
		}
		break;
	}
}

/* ���µ�һ�д�ӡ */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

	/* �����һ��ֵ����ͷ����� */
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	/* ������м��� */
	if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "^") == 0) { return lval_num(pow(x.num , y.num)); }
	if (strcmp(op, "m") == 0) { return lval_num(max(x.num, y.num)); }
	if (strcmp(op, "x") == 0) { return lval_num(min(x.num, y.num)); }
	if (strcmp(op, "/") == 0) {
		/* ����ڶ����������򷵻ش��� */
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num / y.num);
	}
	if (strcmp(op, "%") == 0) {
		/* ����ڶ����������򷵻ش��� */
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num % y.num);
	}

	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

	if (strstr(t->tag, "number")) {
		/* �����ֵ���������������Χerrno == ERANGE ,���ش���*/
		errno = 0;
		long x = strtol(t->contents, NULL, 10); //���� strtod() �������ַ���ת����˫���ȸ�����(double)
		double strtod(const char* str, char** endptr);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	char* op = t->children[1]->contents;
	lval x = eval(t->children[2]);

	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char** argv) {

	/*����������*/
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	/*�����﷨����  \-?\d+(\.\d+)? */
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | '^'|'m'|'x';      \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
		Number, Operator, Expr, Lispy);

	puts("��ӭʹ�õݹ������");
	puts("Press Ctrl+c to Exit\n");

	while (1) {

		char* input = readline("lispy> ");
		add_history(input);

		mpc_result_t r;
		/*�������ʽ׼ȷ���������������ͷſռ�*/
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);
		}
		/*�������ʽ�����򱨴���ɾ��*/
		else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		/*�ͷ�����*/
		free(input);

	}
	/*�����ڴ�*/
	mpc_cleanup(4, Number, Operator, Expr, Lispy);

	return 0;
}