#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

void scan();

typedef enum {
	// Scanning
	ADD, SUB, MUL, DIV, NUM, END,
	OPAREN, CPAREN,

	// Other
	NEG
} symbol;

typedef struct {
	symbol type;
	double val;
} token;

typedef struct {
	token tok[256];
	int idx;
	int cur;
} toks;

toks tokens;
void push_token(symbol s, double v) {
	tokens.tok[tokens.idx].type = s;
	tokens.tok[tokens.idx].val = v;
	tokens.idx += 1;
}

void print_tokens();

void reset() {
	tokens.idx = 0;
	tokens.cur = 0;
}

size_t mem_size = 512 * 1024 * sizeof(char);
size_t mem_used = 0;
void* mem;

#define IN_BUF_SIZE 4096
char buffer[IN_BUF_SIZE];

typedef struct expr {
	symbol type;
	struct expr *l, *r;
	double val;
} expr;

expr* alloc() {
	expr* ret = (expr*)(mem + mem_used);
	mem_used += sizeof(expr);
	return ret;
}

expr* uni_expr(symbol op, expr *l) {
	expr *e = alloc();
	e->type = op;
	e->l = l;
	return e;
}

expr* bin_expr(symbol op, expr *l, expr *r) {
	expr *e = alloc();
	e->type = op;
	e->l = l;
	e->r = r;
	return e;
}

expr* num_expr(double val) {
	expr *e = alloc();
	e->type = NUM;
	e->val = val;
	return e;
}

token* peek() {
	return &tokens.tok[tokens.cur];
}

void next() {
	tokens.cur += 1;
}

expr* muldiv();
expr* unary();
expr* number();
expr* addsub();

expr* expression() {
	expr* e = addsub();

	if (peek()->type != END) {
		fprintf(stderr, "err: expected end of expression, got: '%d'\n", peek()->type);
		exit(EXIT_FAILURE);
	}
	return e;
}

expr* addsub() {
	expr *e = muldiv();

	while (peek()->type == ADD || peek()->type == SUB) {
		symbol t = peek()->type;
		next();
		expr *r = muldiv();
		e = bin_expr(t, e, r);
	}

	return e;
}


expr* muldiv() {
	expr *e = unary();

	while (peek()->type == MUL || peek()->type == DIV) {
		symbol t = peek()->type;
		next();
		expr *r = unary();
		e = bin_expr(t, e, r);
	}

	return e;
}

expr* unary() {
	if (peek()->type == SUB) {
		symbol t = peek()->type;
		next();

		return uni_expr(NEG, number());
	}

	return number();
}

expr* number() {
	token *t = peek();
	if (t->type == OPAREN) {
		next();
		expr* e = addsub();
		if (peek()->type != CPAREN) {
			fprintf(stderr, "err: closed paren expected, got: '%d'\n", t->type);
			exit(EXIT_FAILURE);
		}
		next();
		return e;
	}

	next();
	return num_expr(t->val);
}

double eval(expr *e) {
	switch (e->type) {
		case NUM:
			return e->val;
		case NEG:
			return (-1.0) * eval(e->l);
		case MUL:
			return eval(e->l) * eval(e->r);
		case DIV:
			return eval(e->l) / eval(e->r);
		case SUB:
			return eval(e->l) - eval(e->r);
		case ADD:
			return eval(e->l) + eval(e->r);
		default:
			perror("unsupported expr type");
			exit(EXIT_FAILURE);
	}
}

int main(int argc, char** argv) {
	if ((mem = malloc(mem_size)) == NULL) {
		perror("unable to allocate memory");
		return EXIT_FAILURE;
	}

	reset();
	scan();

	// Parse
	if (argc > 1) {
		print_tokens();
	}
	expr *e = expression();
	printf("%f\n", eval(e));

	return 0;
}

#define is_number(c) (c >= 48 && c <= 57 || c == '.')

void scan() {
	size_t i, j, n;
	char num_buf[32];

	// Scan and tokenize
	while (n = read(0, buffer, IN_BUF_SIZE)) {
		for (i = 0; i < n; i++) {
			switch (buffer[i]) {
				case ' ':
					continue;
				case '\n':
				case '\r':
					goto stop_scan;
				case '+':
					push_token(ADD, -1);
					continue;
				case '-':
					push_token(SUB, -1);
					continue;
				case '*':
					push_token(MUL, -1);
					continue;
				case '/':
					push_token(DIV, -1);
					continue;
				case '(':
					push_token(OPAREN, -1);
					continue;
				case ')':
					push_token(CPAREN, -1);
					continue;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					for (j = i; is_number(buffer[j]); j++) {
						num_buf[j - i] = buffer[j];
					}
					num_buf[j - i] = '\0';
					i = j - 1;
					push_token(NUM, atof(num_buf));
					continue;
				default:
					printf("unrecognized character: '%c'\n", buffer[i]);
					continue;
			}
		}
	}
stop_scan:
	push_token(END, -1);
}

void print_tokens() {
	for (int i = 0; i < tokens.idx; i++) {
		switch (tokens.tok[i].type) {
			case OPAREN:
				puts("OPAREN");
				continue;
			case CPAREN:
				puts("CPAREN");
				continue;
			case ADD:
				puts("ADD");
				continue;
			case MUL:
				puts("MUL");
				continue;
			case DIV:
				puts("DIV");
				continue;
			case SUB:
				puts("SUB");
				continue;
			case NUM:
				printf("NUM: %lf\n", tokens.tok[i].val);
				continue;
			case END:
				puts("END");
				continue;
		}
	}
}
