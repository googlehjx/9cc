#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the kind of token 
typedef enum {
  TK_RESERVED, // operating symbal
  TK_NUM,      // numerial symbal
  TK_EOF,      // the end symbal
} TokenKind;

typedef struct Token Token;

// The info of Token, including a item of TokenKind, and a pointer pointing to the next node 
struct Token {
  TokenKind kind; // token kind: symbal "+/-" or number
  Token *next;    // pointing to next node
  int val;        // if it's a number, this item has meaning
  char *str;      // the start of the string comprising this token
};

// a global pointer, representing the whole chain of tokens
Token *token;
// a global pointer, represnting the whole original input string
char *user_input;

// error print funciton
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// error print fucntion with location info
void error_at(char* loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}


// 建立一個新的標記，連結到cur
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// tokenize the input string, getting a list of tokens
Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;
  
  while (*p) {
    // skip white spaces
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (strchr("+-*/()", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "invalid token");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

// 抽象語法樹結點的種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // 整數
} NodeKind;

typedef struct Node Node;

// 抽象語法樹結點的結構
struct Node {
  NodeKind kind; // 結點的型態
  Node *lhs;     // 左邊
  Node *rhs;     // 右邊
  int val;       // 只在kind為ND_NUM時使用
};

Node* new_node(NodeKind kind){
	Node* node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

// expecting an operator symbal token, and return true o return false 
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 下一個標記為符合預期的符號時，讀入一個標記並往下繼續。
// 否則警告為錯誤。
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error("不是'%c'", op);
  token = token->next;
}

// 下一個標記為數值時，讀入一個標記並往下繼續，回傳該數值。
// 否則警告為錯誤。
int expect_number() {
  if (token->kind != TK_NUM)
    error_at(token->str, "不是數值");
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() {
  return token->kind == TK_EOF;
}


// 声明
Node* expr();
Node* mul();
Node* unary();
Node* primary();

Node *expr() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_binary(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_binary(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? unary
Node* unary(){
	if(consume('+'))
		return primary();
	if(consume('-'))
		return new_binary(ND_SUB, new_num(0), primary());
	return primary();
}

Node *primary() {
  // 下一個標記如果是"("，則應該是"(" expr ")"
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  // 否則應該為數值
  return new_num(expect_number());
}

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of argments\n", argv[0]);
    return 1;
  }

  
  user_input = argv[1];
  // 標記解析
  token = tokenize(user_input);
  Node *node = expr();

  // 輸出前半部份組合語言指令
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  

  // 一邊爬抽象語法樹一邊產出指令
  gen(node);

  // 整個算式的結果應該留在堆疊頂部
  // 將其讀到RAX作為函式的返回值
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}