#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONT 1
#define DONE 0

#define LEX_UNDEF 0
#define LEX_ZERO 1
#define LEX_TRUE 2
#define LEX_FALSE 3
#define LEX_SUCC 4
#define LEX_PRED 5
#define LEX_ISZERO 6
#define LEX_IF 7
#define LEX_THEN 8
#define LEX_ELSE 9
#define LEX_PAREN_L 10
#define LEX_PAREN_R 11
#define LEX_EOF 12

typedef struct ast_s ast_t;

typedef struct lex_token_s {
    char *sym;
    int typ;
} lex_token_t;

typedef struct lex_token_stream_s lex_token_stream_t;
struct lex_token_stream_s {
    lex_token_t *token;
    lex_token_stream_t *next;
};

typedef struct lex_state {
    FILE *fp;
    char *src;
    char *start;
    int pos;
    int len;
    int err;
    char *err_msg;
    lex_token_stream_t *head;
    lex_token_stream_t *stream;
} lex_state;

typedef struct parser_stack_s parser_stack_t;
struct parser_stack_s {
    ast_t *data;
    parser_stack_t *pred;
};

typedef struct parser_state {
    parser_stack_t *s;
    lex_token_stream_t *stream;
    int err;
    char *err_msg;
} parser_state;

typedef struct { int typ; } s1_t;

typedef struct {
    int typ;
    ast_t *t;
} s2_t;

typedef struct {
    int typ;
    ast_t *p;
    ast_t *right;
    ast_t *left;
} s3_t;

#define AST_S1 1
#define AST_S2 2
#define AST_S3 3

struct ast_s {
    int typ;
    union {
        s1_t *s1;
        s2_t *s2;
        s3_t *s3;
    } data;
};

#define INFO_WARN 0
#define INFO_NUM 1
#define INFO_BOOL 2
#define INFO_TRUE 3
#define INFO_FALSE 4

typedef struct run_info_s {
    int typ;
    union {
        int b;
        int n;
    } data;
    int err;
} run_info_t;

int lex_emit(lex_state *state, int typ) {
    lex_token_t *token = malloc(sizeof(lex_token_t));
    lex_token_stream_t *stream = NULL;

    token->typ = typ;
    token->sym = malloc(sizeof(char) * (state->len + 1));
    strncpy(token->sym, state->start + state->pos, state->len);

    state->stream->token = token;
    if (typ == LEX_EOF) {
        return DONE;
    }

    stream = malloc(sizeof(lex_token_stream_t));
    state->stream->next = stream;
    state->stream = stream;
    state->start = state->start + state->pos + state->len;
    state->pos = 0;
    state->len = 0;
    return CONT;
}

#define CMP(state, x) \
    (strncmp((x), (state)->start + (state)->pos, strlen((x))) == 0)
int lex_text(lex_state *state) {
    switch (*(state->start + state->pos)) {
        case '\0':
            state->len++;
            lex_emit(state, LEX_EOF);
            return DONE;
        case ' ':
            state->pos++;
            return CONT;
        case '0':
            state->len++;
            return lex_emit(state, LEX_ZERO);
        default:
            break;
    }
    if (CMP(state, "true")) {
        state->len += strlen("true");
        return lex_emit(state, LEX_TRUE);
    }
    if (CMP(state, "false")) {
        state->len += strlen("false");
        return lex_emit(state, LEX_FALSE);
    }
    if (CMP(state, "succ")) {
        state->len += strlen("succ");
        return lex_emit(state, LEX_SUCC);
    }
    if (CMP(state, "pred")) {
        state->len += strlen("pred");
        return lex_emit(state, LEX_PRED);
    }
    if (CMP(state, "iszero")) {
        state->len += strlen("iszero");
        return lex_emit(state, LEX_ISZERO);
    }
    if (CMP(state, "if")) {
        state->len += strlen("if");
        return lex_emit(state, LEX_IF);
    }
    if (CMP(state, "then")) {
        state->len += strlen("then");
        return lex_emit(state, LEX_THEN);
    }
    if (CMP(state, "else")) {
        state->len += strlen("else");
        return lex_emit(state, LEX_ELSE);
    }

    state->err = 1;
    return DONE;
}

lex_state *lex(char *src) {
    lex_state *state = malloc(sizeof(lex_state));
    int ret = CONT;
    state->fp = NULL;
    state->src = src;
    state->start = src;
    state->pos = 0;
    state->len = 0;
    state->err = 0;
    state->err_msg = NULL;

    state->head = malloc(sizeof(lex_token_stream_t));
    state->stream = state->head;
    state->head->token = NULL;
    state->head->next = NULL;

    while (ret == CONT) {
        ret = lex_text(state);
    }
    if (state->err == 1) {
        return NULL;
    }
    return state;
}

int parser_state_next(parser_state *state) {
    lex_token_stream_t *stream = NULL;
    stream = state->stream;
    state->stream = stream->next;
    free(stream->token->sym);
    free(stream->token);
    free(stream);
    return 0;
}

ast_t *parser_pop(parser_state *state) {
    parser_stack_t *stack = state->s;
    if (stack == NULL) {
        return NULL;
    }
    ast_t *t = stack->data;
    state->s = stack->pred;
    free(stack);
    return t;
}

int parser_push(parser_state *state, ast_t *ast) {
    parser_stack_t *stack = malloc(sizeof(parser_stack_t));
    stack->data = ast;
    stack->pred = state->s;
    state->s = stack;
    return 0;
}

int parse_start(parser_state *state);

int parse_s1(parser_state *state) {
    ast_t *data = malloc(sizeof(ast_t));
    data->typ = AST_S1;
    data->data.s1 = malloc(sizeof(s1_t));
    switch (state->stream->token->typ) {
        case LEX_ZERO:
            data->data.s1->typ = LEX_ZERO;
            break;
        case LEX_TRUE:
            data->data.s1->typ = LEX_TRUE;
            break;
        case LEX_FALSE:
            data->data.s1->typ = LEX_FALSE;
            break;
        default:
            return DONE;
    }
    parser_push(state, data);
    parser_state_next(state);
    return CONT;
}

int parse_s2(parser_state *state) {
    ast_t *data = malloc(sizeof(ast_t)), *t = NULL;
    data->typ = AST_S2;
    data->data.s2 = malloc(sizeof(s2_t));
    switch (state->stream->token->typ) {
        case LEX_ISZERO:
            data->data.s2->typ = LEX_ISZERO;
            break;
        case LEX_PRED:
            data->data.s2->typ = LEX_PRED;
            break;
        case LEX_SUCC:
            data->data.s2->typ = LEX_SUCC;
            break;
    }
    parser_state_next(state);
    parse_start(state);
    t = parser_pop(state);
    if (t == NULL) {
        state->err = 1;
        return DONE;
    }
    data->data.s2->t = t;
    parser_push(state, data);
    return CONT;
}
int parse_s3(parser_state *state) {
    ast_t *data = NULL, *t = NULL;
    int ret = CONT;
    if (state->stream->token->typ != LEX_IF) {
        state->err = 1;
        return DONE;
    }
    parser_state_next(state);

    ret = parse_start(state);
    if (ret == DONE) {
        state->err = 1;
        return DONE;
    }

    t = parser_pop(state);
    if (t == NULL) {
        state->err = 1;
        return DONE;
    }

    data = malloc(sizeof(ast_t));
    data->typ = AST_S3;
    data->data.s3 = malloc(sizeof(s3_t));
    data->data.s3->p = t;

    if (state->stream->token->typ != LEX_THEN) {
        state->err = 1;
        return DONE;
    }
    parser_state_next(state);

    ret = parse_start(state);
    if (ret == DONE) {
        state->err = 1;
        return DONE;
    }
    t = parser_pop(state);

    if (t == NULL) {
        state->err = 1;
        return DONE;
    }
    data->data.s3->right = t;

    if (state->stream->token->typ != LEX_ELSE) {
        state->err = 1;
        return DONE;
    }
    parser_state_next(state);

    ret = parse_start(state);
    if (ret == DONE) {
        state->err = 1;
        return DONE;
    }

    t = parser_pop(state);
    if (t == NULL) {
        state->err = 1;
        return DONE;
    }
    data->data.s3->left = t;

    return DONE;
}

int parse_start(parser_state *state) {
    if (state == NULL) {
        printf("null state\n");
        return DONE;
    }
    if (state->stream == NULL) {
        printf("null stream\n");
        return DONE;
    }
    if (state->stream->token == NULL) {
        printf("null token\n");
        return DONE;
    }
    switch (state->stream->token->typ) {
        case LEX_ZERO:
        case LEX_TRUE:
        case LEX_FALSE:
            return parse_s1(state);
        case LEX_SUCC:
        case LEX_PRED:
        case LEX_ISZERO:
            return parse_s2(state);
        case LEX_IF:
            return parse_s3(state);
        case LEX_THEN:
        case LEX_ELSE:
            state->err = 1;
            return DONE;
        case LEX_EOF:
            return DONE;
        default:
            state->err = 1;
            return DONE;
    }
    parser_state_next(state);
    return DONE;
}

parser_state *parse(lex_state *lexer) {
    parser_state *state = malloc(sizeof(parser_state));
    state->s = NULL;
    state->stream = lexer->head;
    parse_start(state);
    if (state == NULL) {
        return NULL;
    }

    if (state->err == 1) {
        return NULL;
    }
    return state;
}

run_info_t *run(ast_t *ast);

run_info_t *run_s1(s1_t *s1) {
    run_info_t *info = NULL;
    if (s1 == NULL) {
        return NULL;
    }
    switch (s1->typ) {
        case LEX_ZERO:
            info = malloc(sizeof(run_info_t));
            info->typ = INFO_NUM;
            info->data.n = 0;
            break;
        case LEX_TRUE:
            info = malloc(sizeof(run_info_t));
            info->typ = INFO_BOOL;
            info->data.b = INFO_TRUE;
            break;
        case LEX_FALSE:
            info = malloc(sizeof(run_info_t));
            info->typ = INFO_BOOL;
            info->data.b = INFO_FALSE;
            break;
        default:
            return NULL;
    }
    return info;
}

run_info_t *run_s2(s2_t *s2) {
    run_info_t *info = NULL;
    switch (s2->typ) {
        case LEX_ISZERO:
            info = run(s2->t);
            if (info->typ != INFO_NUM) {
                return NULL;
            }
            if (info->data.n == 0) {
                info->typ = INFO_BOOL;
                info->data.b = INFO_TRUE;
            } else {
                info->typ = INFO_BOOL;
                info->data.b = INFO_FALSE;
            }
            break;
        case LEX_PRED:
            info = run(s2->t);
            if (info->typ != INFO_NUM) {
                return NULL;
            }
            info->data.n--;
            break;
        case LEX_SUCC:
            info = run(s2->t);
            if (info->typ != INFO_NUM) {
                return NULL;
            }
            info->data.n++;
            break;
        default:
            return NULL;
    }
    return info;
}

run_info_t *run_s3(s3_t *s3) {
    run_info_t *info = NULL, *p = NULL, *right = NULL, *left = NULL;
    switch (s3->typ) {
        case LEX_IF:
            p = run(s3->p);
            if (p->typ != INFO_BOOL) {
                return NULL;
            }
            if (p->data.b == INFO_TRUE) {
                right = run(s3->right);
                if (right == NULL) {
                    return NULL;
                }
            } else {
                left = run(s3->left);
                if (left == NULL) {
                    return NULL;
                }
            }

        default:
            return NULL;
    }
    return info;
}

int show(run_info_t *info) {
    if (info == NULL) {
        return -1;
    }
    switch (info->typ) {
        case INFO_WARN:
            printf("type mismatch\n");
            return -1;
        case INFO_NUM:
            printf("numeric:%d\n", info->data.n);
            return 0;
        case INFO_BOOL:
            printf("boolean:");
            switch (info->data.b) {
                case INFO_TRUE:
                    printf("true\n");
                    return 0;
                case INFO_FALSE:
                    printf("false\n");
                    return 0;
            }
        default:
            return -1;
    }
    return -1;
}

run_info_t *run(ast_t *ast) {
    run_info_t *info = NULL;
    if (ast == NULL) {
        printf("null ast\n");
        return NULL;
    }
    switch (ast->typ) {
        case AST_S1:
            info = run_s1(ast->data.s1);
            break;
        case AST_S2:
            info = run_s2(ast->data.s2);
            break;
        case AST_S3:
            info = run_s3(ast->data.s3);
            break;
        default:
            return NULL;
    }
    return info;
}

int main(int argc, char **argv) {
    char *name = "arith";
    lex_state *lexer = NULL;
    parser_state *parser = NULL;
    run_info_t *info = NULL;
    if (argc < 2) {
        printf("Usage:%s \"code\"\n", name);
        return -1;
    }
    lexer = lex(argv[1]);
    if (lexer == NULL) {
        printf("fail lexical analysis\n");
        return -1;
    }
    if (lexer->err == 1) {
        printf("fail lexical analysis\n");
        return -1;
    }
    parser = parse(lexer);
    if (parser == NULL) {
        printf("fail parse analysis\n");
        return -1;
    }
    if (parser->err == 1) {
        printf("fail parse analysis\n");
        return -1;
    }
    info = run(parser->s->data);
    if (info == NULL) {
        printf("fail running program: null info\n");
        return -1;
    }
    if (info->err == 1) {
        printf("fail running program: something wrong\n");
        return -1;
    }
    show(info);
    return 0;
}
