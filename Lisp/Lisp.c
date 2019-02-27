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

/* 枚举错误类型 */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* 枚举可能的值类型 */
enum { LVAL_NUM, LVAL_ERR };

/* 声明结构体类型 */
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

/* 打印出结果 */
void lval_print(lval v) {
	switch (v.type) {
		/*如果没错打印结果*/
	case LVAL_NUM: printf("%li", v.num); break;

		/* 如果输入错误 */
	case LVAL_ERR:
		/* 检查是哪一种错误 */
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

/* 在新的一行打印 */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {

	/* 如果有一个值错误就返回它 */
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	/* 否则进行计算 */
	if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if (strcmp(op, "^") == 0) { return lval_num(pow(x.num , y.num)); }
	if (strcmp(op, "m") == 0) { return lval_num(max(x.num, y.num)); }
	if (strcmp(op, "x") == 0) { return lval_num(min(x.num, y.num)); }
	if (strcmp(op, "/") == 0) {
		/* 如果第二个数是零则返回错误 */
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num / y.num);
	}
	if (strcmp(op, "%") == 0) {
		/* 如果第二个数是零则返回错误 */
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num % y.num);
	}

	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

	if (strstr(t->tag, "number")) {
		/* 检查数值溢出错误，若超出范围errno == ERANGE ,返回错误*/
		errno = 0;
		long x = strtol(t->contents, NULL, 10); //函数 strtod() 用来将字符串转换成双精度浮点数(double)
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

	/*创建解释器*/
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");
	/*定义语法规则  \-?\d+(\.\d+)? */
	mpca_lang(MPCA_LANG_DEFAULT,
		"                                                     \
      number   : /-?[0-9]+/ ;                             \
      operator : '+' | '-' | '*' | '/' | '%' | '^'|'m'|'x';      \
      expr     : <number> | '(' <operator> <expr>+ ')' ;  \
      lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
		Number, Operator, Expr, Lispy);

	puts("欢迎使用递归计算器");
	puts("Press Ctrl+c to Exit\n");

	while (1) {

		char* input = readline("lispy> ");
		add_history(input);

		mpc_result_t r;
		/*若输入格式准确，则输出结果，后释放空间*/
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);
		}
		/*若输出格式错误，则报错，并删除*/
		else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		/*释放输入*/
		free(input);

	}
	/*清理内存*/
	mpc_cleanup(4, Number, Operator, Expr, Lispy);

	return 0;
}