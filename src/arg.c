#include "arg.h"

void argval_assign(ArgList id, ArgVal *opt, ArgVal val) {
    switch(id) {
        case ARG_INTEGER: {
            *opt->i = *val.i;
        } break;
        case ARG_STRING: {
            *opt->s = *val.s;
        } break;
        case ARG_DOUBLE: {
            *opt->d = *val.d;
        } break;
        case ARG_NONE: break;
        default: assert(0 && "invalid enum"); break;
    }
}

int argopt_parse(ArgOpt *opt, char **str) {
    switch(opt->id) {
        case ARG_INTEGER: {
            assert(0 && "todo implement");
        } break;
        case ARG_DOUBLE: {
            assert(0 && "todo implement");
        } break;
        case ARG_STRING: {
            argval_assign(opt->id, &opt->val, (ArgVal){ .s = str });
            //*opt->val.s = *str;
        } break;
        case ARG_NONE: break;
        default: assert(0 && "invalid enum"); break;
    }
    return 0;
}

void argval_print(ArgList id, ArgVal opt) {
    switch(id) {
        case ARG_STRING: {
            if(opt.s) printf("[%s]", *opt.s);
        } break;
        case ARG_INTEGER: {
            if(opt.i) printf("[%i]", *opt.i);
        } break;
        case ARG_DOUBLE: {
            if(opt.d) printf("[%f]", *opt.d);
        } break;
        case ARG_NONE: break;
        default: assert(0 && "invalid enum"); break;
    }
}

void arg_defaults(Arg *arg) {
    TArgOptItem **item = 0;
    while((item = targopt_iter_all(&arg->positional, item))) {
        argval_assign((*item)->val->id, &(*item)->val->val, (*item)->val->ref );
    }
    item = 0;
    while((item = targopt_iter_all(&arg->options, item))) {
        argval_assign((*item)->val->id, &(*item)->val->val, (*item)->val->ref );
    }
    item = 0;
    while((item = targopt_iter_all(&arg->env, item))) {
        argval_assign((*item)->val->id, &(*item)->val->val, (*item)->val->ref );
    }
}

const char **argv_next(const int argc, const char **argv, const char **iter) {
    ptrdiff_t delta = iter - argv;
    if(delta < argc) {
        const char **arg_next = iter + 1;
        ////printf("DELTA %zi, NEXT: '%s'\n", delta, *arg_next);
        //printf("ARGC %u / PTRDIFF %zi<%zi -> %s\n", argc, delta, delta, *iter);
        return arg_next;
    }
    return 0;
}

void *arg_print_help(void *argp) {
    Arg *arg = argp;
    TArgOptItem **item = 0;
    printf("%s: %s\n\n", arg->name, arg->info);
    /* usage */
    printf("USAGE:\n\n");
    printf("%2s%s", "", arg->name);
    while((item = targopt_iter_all(&arg->positional, item))) {
        printf(" %s", (*item)->val->opt);
    }
    if(arg->options.used) { printf(" [OPTIONS]"); }
    if(arg->rest.allow) {
        printf(" %s", arg->rest.opt);
    }
    printf("\n");
    /* options */
    if(arg->options.used) {
        printf("\nOPTIONS:\n\n");
        while((item = targopt_iter_all(&arg->options, item))) {
            ArgOpt *opt = (*item)->val;
            printf("  -%c  %s  %s ", opt->c, opt->opt, opt->help);
            argval_print(opt->id, opt->val);
            printf("\n");
        }
    }
    /* environment arguments */
    if(arg->env.used) {
        printf("ENVIONMENT ARGUMENTS:\n\n");
        printf("%6s", arg->name);
        while((item = targopt_iter_all(&arg->env, item))) {
            printf("%6s%s %s", "", (*item)->val->opt, (*item)->val->help);
        }
        printf("\n");
    }
    if(strlen(arg->url)) {
        printf("\n%s", arg->url);
    }
    printf("\n");
    return 0;
}


size_t cstr_hash(const char *str) {
    size_t hash = 5381;
    size_t i = 0;
    size_t len = strlen(str);
    while(i < len) {
        unsigned char c = str[i++];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

int cstr_cmp(const char *a, const char *b) {
    return strcmp(a, b);
}

VEC_IMPLEMENT(VrStr, vrstr, const char *, BY_VAL, BASE, 0);
LUT_IMPLEMENT(TArgOpt, targopt, char *, BY_VAL, ArgOpt, BY_REF, cstr_hash, cstr_cmp, 0, 0);

ArgOpt *arg_attach(Arg *arg, TArgOpt *type, unsigned char c, const char *opt, const char *help) {
    ArgOpt opt_set = {
        .opt = opt,
        .c = c,
        .help = help,
    };
    /* check rules */
    if(type == &arg->options) {
        if(strlen(opt) < 2) {
            fprintf(stderr, "cannot have opt len < 2!\n");
            return (void *)-1;
        }
        if(opt[0] != '-' && opt[1] != '-') {
            fprintf(stderr, "opt char 0 and 1 have to be '-'!\n");
            return (void *)-1;
        }
    }
    if(type == &arg->env) {
        if(c) {
            fprintf(stderr, "cannot have -%c for environment!\n", c);
            return (void *)-1;
        }
    }
    if(type == &arg->positional) {
        if(c) {
            fprintf(stderr, "cannot have -%c for positional!\n", c);
            return (void *)-1;
        }
    }
    /* done checking the rules */
    TArgOptItem *kv_v = targopt_once(type, opt, &opt_set);
    if(!kv_v) {
        fprintf(stderr, "failed setting option! %c,%s,%s\n", c, opt, help);
        assert(0);
    }
    ////printf("SET '%s' id:%u\n", kv_v->val->opt, kv_v->val->id);
    if(c) {
        arg->c[c] = kv_v->val;
    }
    return kv_v->val;
    //return (ArgOptRefVal){ kv_r->val, 0 };
}

void arg_attach_help(Arg *arg, unsigned char c, const char *opt, const char *help, const char *name, const char *info, const char *url) {
    arg->name = name;
    arg->info = info;
    arg->url = url;
    ArgOpt *attached = arg_attach(arg, &arg->options, 'h', "--help", "show this help");
    argopt_set_callback(attached, arg_print_help, arg, true);
}

void arg_allow_rest(Arg *arg, const char *opt) {
    arg->rest.allow = true;
    arg->rest.opt = opt;
}

void argopt_set_str(ArgOpt *opt, char **ref, char **val) {
    opt->ref.s = ref;
    opt->val.s = val;
    opt->id = ARG_STRING;
}

void argopt_set_int(ArgOpt *opt, int *ref, int *val) {
    opt->ref.i = ref;
    opt->val.i = val;
    opt->id = ARG_INTEGER;
}

void argopt_set_dbl(ArgOpt *opt, double *ref, double *val) {
    opt->ref.d = ref;
    opt->val.d = val;
    opt->id = ARG_DOUBLE;
}

void argopt_set_callback(ArgOpt *opt, ArgCallback callback, void *data, bool exit_early) {
    opt->callback.function = callback;
    opt->callback.data = data;
    opt->callback.exit_early = exit_early;
}

const char **arg_iter(const int argc, const char **argv, const char **iter) {
next:
    if(!argc) return 0;
    if(argc <= 1) return 0;
    if(!iter) return &argv[1];
    ptrdiff_t diff = iter - argv;
    if(diff + 1 >= argc) return 0;
    const char **result = &argv[diff + 1];
    if(!result) goto next;
    size_t len = strlen(*result);
    if(!len) goto next;
    ////printf("CHECK ARG: '%s'\n", *result);
    return result;
}

int arg_parse(Arg *arg, const int argc, const char **argv, bool *exit_early) {
    
    arg->argc = argc;
    arg->argv = argv;
    //arg_defaults(arg);
    const char **iter = 0;
    while((iter = arg_iter(argc, argv, iter))) {
        size_t len = strlen(*iter);
        ArgOpt *item = 0;
        if(len == 2 && (*iter)[0] == '-') {
            unsigned char c = (*iter)[1];
            item = arg->c[c];
        }
        if(!item) {
            item = targopt_get(&arg->options, *iter);
        }
        if(!item) {
            if(arg->rest.allow) {
                vrstr_push_back(&arg->rest.vstr, *iter);
            }
            continue;
        }
        //printf("HAVE AN ARGUMENT STORED FOR: %c/%s id:%u; move to NEXT!\n", item->c, item->opt, item->id);
        /* move */
        iter = argv_next(argc, argv, iter);
        /* finally, parse value */
        if((!iter || (iter && !*iter)) && item->id) {
            fprintf(stderr, "missing value for: '%s'! id:%u\n", item->opt, item->id);
            return -1;
        }
        if(argopt_parse(item, (char **)iter)) return -1;
        if(item->callback.function) {
            if(item->callback.function(item->callback.data)) return -1;;
            if(item->callback.exit_early) *exit_early = true;
        }
    }

    return 0;
}



