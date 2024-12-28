#include "arg.h"

#if 0

VEC_IMPLEMENT(VrStr, vrstr, const char *, BY_VAL, BASE, 0);
VEC_IMPLEMENT(VStr, vstr, char *, BY_VAL, BASE, 0);
#endif

void argval_assign(ArgList id, ArgVal *opt, ArgVal val) {
    switch(id) {
        case ARG_INTEGER: {
            if(val.dynamic) {
                opt->i = malloc(sizeof(*opt->i));
            }
            *opt->i = *val.i;
        } break;
        case ARG_STRING: {
            if(val.dynamic) {
                opt->s = malloc(sizeof(*opt->s));
            }
            *opt->s = *val.s;
        } break;
        case ARG_DOUBLE: {
            if(val.dynamic) {
                opt->d = malloc(sizeof(*opt->d));
            }
            *opt->d = *val.d;
        } break;
        case ARG_NONE: break;
        default: assert(0 && "invalid enum"); break;
    }
}

int argopt_parse(ArgOpt *opt, char **str) {
    switch(opt->id) {
        case ARG_INTEGER: {
            char *endptr;
            int i = strtoll(*str, &endptr, 0);
            argval_assign(opt->id, &opt->val, (ArgVal){ .i = &i, .dynamic = true });
            //assert(0 && "todo implement");
        } break;
        case ARG_DOUBLE: {
            assert(0 && "todo implement");
        } break;
        case ARG_STRING: {
            argval_assign(opt->id, &opt->val, (ArgVal){ .s = &RSTR_L(*str), .dynamic = true });
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
            if(rstr_length(*opt.s)) printf("[%.*s]", RSTR_F(*opt.s));
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
    printf("%.*s: %.*s\n\n", RSTR_F(arg->name), RSTR_F(arg->info));
    /* usage */
    printf("USAGE:\n\n");
    printf("%2s%.*s", "", RSTR_F(arg->name));
    while((item = targopt_iter_all(&arg->positional, item))) {
        printf(" %.*s", RSTR_F((*item)->val->opt));
    }
    if(arg->options.used) { printf(" [OPTIONS]"); }
    if(arg->rest.allow) {
        printf(" %.*s", RSTR_F(arg->rest.opt));
    }
    printf("\n");
    /* options */
    if(arg->options.used) {
        printf("\nOPTIONS:\n\n");
        while((item = targopt_iter_all(&arg->options, item))) {
            ArgOpt *opt = (*item)->val;
            printf("  -%c  %.*s  %.*s ", opt->c, RSTR_F(opt->opt), RSTR_F(opt->help));
            argval_print(opt->id, opt->val);
            printf("\n");
        }
    }
    /* environment arguments */
    if(arg->env.used) {
        printf("ENVIONMENT ARGUMENTS:\n\n");
        printf("%6s%.*s", "", RSTR_F(arg->name));
        while((item = targopt_iter_all(&arg->env, item))) {
            printf("%6s%.*s %.*s", "", RSTR_F((*item)->val->opt), RSTR_F((*item)->val->help));
        }
        printf("\n");
    }
    if(rstr_length(arg->url)) {
        printf("\n%.*s", RSTR_F(arg->url));
    }
    printf("\n");
    return 0;
}

LUT_IMPLEMENT(TArgOpt, targopt, RStr, BY_REF, ArgOpt, BY_REF, rstr_phash, rstr_cmp, 0, 0);

ArgOpt *arg_attach(Arg *arg, TArgOpt *type, unsigned char c, const char *opt, const char *help) {
    ArgOpt opt_set = {
        .opt = RSTR_L(opt),
        .c = c,
        .help = RSTR_L(help),
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
    TArgOptItem *kv_v = targopt_once(type, &opt_set.opt, &opt_set);
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
    arg->name = RSTR_L(name);
    arg->info = RSTR_L(info);
    arg->url = RSTR_L(url);
    ArgOpt *attached = arg_attach(arg, &arg->options, 'h', "--help", "show this help");
    argopt_set_callback(attached, arg_print_help, arg, true);
}


void arg_allow_rest(Arg *arg, const char *opt) {
    arg->rest.allow = true;
    arg->rest.opt = RSTR_L(opt);
}

void argopt_set_str(ArgOpt *opt, RStr *ref, RStr *val) {
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
    arg_defaults(arg);
    const char **iter = 0;
    while((iter = arg_iter(argc, argv, iter))) {
        RStr it = RSTR_L(*iter);
        ArgOpt *item = 0;
        if(rstr_length(it) == 2 && (*iter)[0] == '-') {
            unsigned char c = (*iter)[1];
            item = arg->c[c];
        }
        if(!item) {
            item = targopt_get(&arg->options, &it);
        }
        if(!item) {
            if(arg->rest.allow) {
                vrstr_push_back(&arg->rest.vstr, &it);
            }
            continue;
        }
        //printf("HAVE AN ARGUMENT STORED FOR: %c/%s id:%u; move to NEXT!\n", item->c, item->opt, item->id);
        /* move */
        iter = argv_next(argc, argv, iter);
        /* finally, parse value */
        if((!iter || (iter && !*iter)) && item->id) {
            fprintf(stderr, "missing value for: '%.*s'! id:%u\n", RSTR_F(item->opt), item->id);
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


