#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "arg.h"

VEC_IMPLEMENT(VBool, vbool, bool, BY_VAL, BASE, 0);
VEC_IMPLEMENT(VBool, vbool, bool, BY_VAL, ERR);
VEC_IMPLEMENT(VSize, vsize, bool, BY_VAL, BASE, 0);
VEC_IMPLEMENT(VSize, vsize, bool, BY_VAL, ERR);
VEC_IMPLEMENT(VDouble, vdouble, bool, BY_VAL, BASE, 0);
VEC_IMPLEMENT(VDouble, vdouble, bool, BY_VAL, ERR);
//LUT_IMPLEMENT(TArgVal, targval, ArgVal, BY_REF, void *, BY_VAL, rstr_hash, rstr_cmp, 0, 0);
LUT_IMPLEMENT(TArgOpt, targopt, RStr, BY_REF, ArgOpt, BY_REF, rstr_phash, rstr_cmp, 0, argopt_free);

VEC_IMPLEMENT(VpArgOpt, vpargopt, ArgOpt *, BY_VAL, BASE, 0);
VEC_IMPLEMENT(VpArgOpt, vpargopt, ArgOpt *, BY_VAL, ERR);

#if 0
LUT_IMPLEMENT(TArgVar, targvar, RStr, BY_REF, ArgVar, BY_REF, rstr_hash, rstr_cmp, 0, argvar_free);

VEC_IMPLEMENT(VpArgVar, vpargvar, ArgVar *, BY_VAL, BASE, 0);
VEC_IMPLEMENT(VpArgVar, vpargvar, ArgVar *, BY_VAL, ERR);
#endif

void argopt_free(ArgOpt *opt) { /*{{{*/
    ASSERT_ARG(opt);
    //printff("FREE %p (%.*s) %p", opt, RSTR_F(opt->str), opt->options);
    if(opt->value && opt->ninput != 1) {
        switch(opt->id) {
            case ARG_INT: { vsize_free(&opt->value->vz); } break;
            case ARG_BOOL: { vbool_free(&opt->value->vb); } break;
            case ARG_FLOAT: { vdouble_free(&opt->value->vf); } break;
            case ARG_STRING: { vrstr_free(&opt->value->vs); } break;
            default: break;
        }
    }
    if(opt->options) {
        vpargopt_free(&opt->options->array);
        targopt_free(&opt->options->table);
        free(opt->options);
        //opt->options = 0;
    }
    memset(opt, 0, sizeof(*opt));
    //targval_free(&opt->options);
} /*}}}*/

#if 0
void argvar_free(ArgVar *var) { /*{{{*/
    ASSERT_ARG(var);
} /*}}}*/
#endif

RStr argopt_list(ArgOpt *opt) { /*{{{*/
    ASSERT_ARG(opt);
    if(opt->ninput == 1) {
        switch(opt->id) {
            case ARG_INT: return RSTR("<int>");
            case ARG_BOOL: return RSTR("<bool>");
            case ARG_NONE: return RSTR("");
            case ARG_FLOAT: return RSTR("<float>");
            case ARG_STRING: return RSTR("<string>");
            case ARG_OPTION: return RSTR("<option>");
        }
    } else {
        switch(opt->id) {
            case ARG_INT: return RSTR("<int-vec>");
            case ARG_BOOL: return RSTR("<bool-vec>");
            case ARG_NONE: return RSTR("");
            case ARG_FLOAT: return RSTR("<float-vec>");
            case ARG_STRING: return RSTR("<string-vec>");
            case ARG_OPTION: return RSTR("<option-vec>");
        }
    }
    return RSTR("<<<invalid>>>");
} /*}}}*/

RStr arg_list(ArgList id) { /*{{{*/
    switch(id) {
        case ARG_INT: return RSTR("<int>");
        case ARG_BOOL: return RSTR("<bool>");
        case ARG_NONE: return RSTR("");
        case ARG_FLOAT: return RSTR("<float>");
        case ARG_STRING: return RSTR("<string>");
        case ARG_OPTION: return RSTR("<option>");
    }
    return RSTR("<<<invalid>>>");
} /*}}}*/

void argval_print(ArgOpt *opt) { /*{{{*/
    ASSERT_ARG(opt);
    if(opt->ninput != 1) ASSERT(0, "TODO:IMPLEMENT");
    if(opt->fallback) {
        switch(opt->id) {
            /* TODO: check array */
            case ARG_INT: { printf(F("%zu", FG_GN), opt->value->z); } break;
            case ARG_STRING: { printf(F("%.*s", FG_GN), RSTR_F(opt->value->s)); } break;
            case ARG_FLOAT: { printf(F("%f", FG_GN), opt->value->f); } break;
            case ARG_BOOL: { printf(F("%s", FG_GN), opt->value->b ? "true" : "false"); } break;
            case ARG_OPTION: { 
                if(opt->options && vpargopt_length(opt->options->array)) {
                    ArgOpt *option = vpargopt_get_front(&opt->options->array);
                    printf(F("%.*s", FG_GN), RSTR_F(option->str));
                } break;
                default: break;
            }
        }
    }
} /*}}}*/

#if 0
void argvar_help(VpArgVar *array, int *padding, bool is_last, bool is_positional, bool do_print) { /*{{{*/
    ASSERT_ARG(array);
    ASSERT_ARG(padding);
    /* display all variables */
    for(size_t i = 0; i < vpargvar_length(*array); ++i) {
        ArgVar *var = vpargvar_get_at(array, i);
        int l = rstr_length(var->str) + rstr_length(arg_list(var->id));
        int pad = *padding - l;
        if(pad < 0) {
            if(do_print) ASSERT(0, ERR_UNREACHABLE ", trying to pad negatively (%i)", pad);
            *padding = l;
        }

        if(do_print) {
            printf("%6s" F("%.*s", FG_WT_B BOLD) " " F("%.*s", FG_CY_B) "%*s   %s%.*s", "",
                    RSTR_F(var->str), 
                    RSTR_F(arg_list(var->id)), 
                    pad, "", 
                    is_positional && var->fallback ? F("<optional>", FG_YL_B) " " : "",
                    RSTR_F(var->help));
            printf("\n");
        }
    }
} /*}}}*/
#endif

void argobj_help(VpArgOpt *array, int *padding, int nest, bool is_last, bool is_positional, bool do_print) { /*{{{*/
    ASSERT_ARG(array);
    ASSERT_ARG(padding);
    /* display all arguments */
    for(size_t i = 0; i < vpargopt_length(*array); ++i) {
        ArgOpt *opt = vpargopt_get_at(array, i);
        int l = rstr_length(opt->str) + rstr_length(argopt_list(opt));
        int pad = *padding - 2 * nest - l;
        if(pad < 0) {
            if(do_print) ASSERT(0, ERR_UNREACHABLE ", trying to pad negatively (%i).. did the programmer forget to run the exact same function exactly the same but with do_print=false?", pad);
            *padding = 2 * nest + l;
        }

        if(do_print) {
            if(nest) {
                printf("%8s", "");
                for(int j = 1; j < nest; ++j) { printf(F("%s", FG_CY), is_last ? "  " : ARG_ART_LINE); }
                printf(F("%s", FG_CY_B) "" F("%.*s", FG_BL_B) " " F("%.*s", FG_CY_B) "%*s %.*s" "", 
                        i + 1 < vpargopt_length(*array) ? (!i ? ARG_ART_TOP : ARG_ART_TSHAPE) : ARG_ART_BOT, 
                        RSTR_F(opt->str), 
                        RSTR_F(argopt_list(opt)), 
                        pad, "", 
                        RSTR_F(opt->help));
            } else {
                printf("  " F("%c%c  %.*s", FG_WT_B BOLD) " " F("%.*s", FG_CY_B) "%*s   %s%.*s", 
                        opt->c ? '-' : ' ', 
                        opt->c ? opt->c : ' ', 
                        RSTR_F(opt->str), 
                        RSTR_F(argopt_list(opt)), 
                        pad, "", 
                        is_positional && opt->fallback ? F("<optional>", FG_YL_B) " " : "",
                        RSTR_F(opt->help));
            }
            if(opt->fallback && opt->id != ARG_OPTION) {
                printf(F(" =", FG_BK_B));
                argval_print(opt);
            }
            if(opt->value && opt->id == ARG_OPTION && opt->options && vpargopt_length(opt->options->array)) {
                for(size_t j = 0; j < vpargopt_length(opt->options->array); ++j) {
                    ArgOpt *item = vpargopt_get_at(&opt->options->array, j);
                    //printff("GET %.*s / %zu @ %zu/%zu", RSTR_F(opt->str), opt->value->z, j, vpargopt_length(opt->options->array));
                    if(item->index == (size_t)opt->value->z) {
                        printf(F(" =", FG_BK_B) F("%.*s", FG_GN), RSTR_F(item->str));
                        break;
                    }
                    if(j + 1 >= vpargopt_length(opt->options->array)) {
                        ASSERT_ERROR("no matching index found!");
                    }
                }
            }
            printf("\n");
        }
        /* display potential options */
        if(opt->id == ARG_OPTION && opt->options) {
            argobj_help(&opt->options->array, padding, nest + 1, i + 1 >= vpargopt_length(*array), is_positional, do_print);
        }
    }
} /*}}}*/

int arg_help(void *arg_v) { /*{{{*/
    ASSERT_ARG(arg_v);
    Arg *arg = arg_v;

    /* header */
    printf(F("%.*s", BOLD) ": %.*s\n\n", RSTR_F(arg->name), RSTR_F(arg->info));

    /* usage */
    printf(F("USAGE:", UL) "\n" F("%6s%.*s", BOLD) " [OPTIONS] ", "", RSTR_F(arg->name));
    if(arg->rest.allow) {
        printf("%.*s", RSTR_F(arg->rest.info));
    }
    if(vpargopt_length(arg->positional.array)) {
        for(size_t i = 0; i < vpargopt_length(arg->positional.array); ++i) {
            ArgOpt *opt = vpargopt_get_at(&arg->positional.array, i); 
            if(!opt->fallback) {
                printf("%.*s ", RSTR_F(opt->str));
            } else {
                printf(F("(%.*s) ", ), RSTR_F(opt->str));
            }
        }
    }
    printf("\n\n");

    /* estimate padding */
    int padding = 0;
    argobj_help(&arg->positional.array, &padding, 0, false, true, false);
    argobj_help(&arg->options.array, &padding, 0, false, false, false);
    argobj_help(&arg->environment.array, &padding, 0, false, true, false);

    /* positional arguments */
    if(vpargopt_length(arg->positional.array)) {
        //printf(F("POSITIONAL:", UL) "\n");
        printf(F("%.*s", UL) "\n", RSTR_F(arg->positional.info));
        argobj_help(&arg->positional.array, &padding, 0, false, true, true);
        printf("\n");
    }

    /* options */
    if(vpargopt_length(arg->options.array)) {
        //printf(F("OPTIONS:", UL) "\n");
        printf(F("%.*s", UL) "\n", RSTR_F(arg->options.info));
        argobj_help(&arg->options.array, &padding, 0, false, false, true);
        printf("\n");
    }

    /* environment variables */
    //if(vpargvar_length(arg->environment.array)) {
    if(vpargopt_length(arg->environment.array)) {
        //printf(F("ENVIRONMENT:", UL) "\n");
        printf(F("%.*s", UL) "\n", RSTR_F(arg->environment.info));
        argobj_help(&arg->environment.array, &padding, 0, false, false, true);
        printf("\n");
    }

    printf(F("%.*s", UL FG_BL) "\n", RSTR_F(arg->url));

    arg->exit_early = true;
    return 0;
} /*}}}*/

int arg_version(void *arg_v) { /*{{{*/
    ASSERT_ARG(arg_v);
    Arg *arg = arg_v;
    printf("%.*s version %s\n", RSTR_F(arg->name), arg->version);
    arg->exit_early = true;
    return 0;
} /*}}}*/

ErrImpl argopt_attach_option(ArgOpt *arg, ArgOpt *opt) { /*{{{*/
    ASSERT_ARG(arg);
    if(arg->id != ARG_OPTION) THROW("we should only attach to ARG_OPTION!");
    if(!arg->options) {
        arg->options = malloc(sizeof(*arg->options));
        memset(arg->options, 0, sizeof(*arg->options));
    }
    TRYG(arg_attach(arg->options, opt, 0));
    return 0;
error:
    return -1;
} /*}}}*/

#define ERR_argopt_assign_value(...)    "failed assigning value"
ErrImplStatic argopt_assign_value(bool run, ArgOpt *opt, ArgVal *val) /*{{{*/
{
    if(run) return 0;
    ASSERT_ARG(opt);
    ASSERT_ARG(val);
    if(opt->value) {
        switch(opt->id) {
            case ARG_INT: case ARG_OPTION: {
                if(opt->ninput == 1) opt->value->z = val->z;
                else if(!opt->ninput) TRYG(vsize_push_back(&opt->value->vz, val->z));
                else if(vsize_length(opt->value->vz) + 1 < opt->ninput) TRYG(vsize_push_back(&opt->value->vz, val->z));
                else THROW("too many arguments provided, maximum %zu allowed", opt->ninput);
            } break;
            case ARG_BOOL: {
                if(opt->ninput == 1) opt->value->b = val->b;
                else if(!opt->ninput) TRYG(vbool_push_back(&opt->value->vb, val->b));
                else if(vbool_length(opt->value->vb) + 1 < opt->ninput) TRYG(vbool_push_back(&opt->value->vb, val->b));
                else THROW("too many arguments provided, maximum %zu allowed", opt->ninput);
            } break;
            case ARG_FLOAT: {
                if(opt->ninput == 1) opt->value->f = val->f;
                else if(!opt->ninput) TRYG(vdouble_push_back(&opt->value->vf, val->f));
                else if(vdouble_length(opt->value->vf) + 1 < opt->ninput) TRYG(vdouble_push_back(&opt->value->vf, val->f));
                else THROW("too many arguments provided, maximum %zu allowed", opt->ninput);
            } break;
            case ARG_STRING: {
                if(opt->ninput == 1) opt->value->s = val->s;
                else if(!opt->ninput) TRYG(vrstr_push_back(&opt->value->vs, &val->s));
                else if(vrstr_length(opt->value->vs) + 1 < opt->ninput) TRYG(vrstr_push_back(&opt->value->vs, &val->s));
                else THROW("too many arguments provided, maximum %zu allowed", opt->ninput);
            } break;
            default: THROW(ERR_UNREACHABLE "; id %u has no handler to set", opt->id);
        }
    } else {
        //THROW("not assigning %.*s for '%.*s' to anything", RSTR_F(argopt_list(opt)), RSTR_F(opt->str));
    }
    return 0;
error:
    return -1;
} /*}}}*/

/* TODO: doing --version=asdf should fail */
ErrImpl arg_parse_val(Arg *arg, size_t argc, const char **argv, ArgOpt *opt, RStr arg_rest, size_t *i, bool *deep_error, bool run) { /*{{{*/
    ASSERT_ARG(arg);
    //ASSERT_ARG(i);
    ASSERT_ARG(opt);
    //printff("PARSING %.*s %.*s %zu -> %s", RSTR_F(opt->str), RSTR_F(argopt_list(opt)), *i, argv[*i]);
    bool deep_error_real = false;
    if(!deep_error) deep_error = &deep_error_real;
    arg->options.info = RSTR("OPTIONS:");
    arg->positional.info = RSTR("POSITIONAL:");
    arg->environment.info = RSTR("ENVIRONMENT:");
    char *endptr = 0;
    RStr check_true[5] = {RSTR("1"), RSTR("y"), RSTR("yes"), RSTR("true"), RSTR("enable")};
    RStr check_false[5] = {RSTR("0"), RSTR("n"), RSTR("no"), RSTR("false"), RSTR("disable")};
    /* figure out what we're actually interested in */
    RStr observe123 = {0};
    RStr observe = {0};
    bool have_vector = false;
    bool have_assignment = false;
    if(run) {
        if(opt->ninput == 1) {
            if(opt->consumed) THROW("already consumed an argument matching this");
        }
        else if(opt->ninput == 0) { /* do nothing */ }
        else if(opt->consumed && (opt->consumed >= opt->ninput)) THROW("no more values allowed (max %zu)", opt->ninput);
    }
    if(opt->id != ARG_NONE) {
        if(rstr_length(arg_rest) && rstr_get_front(&arg_rest) == '=') {
            observe123 = RSTR_I0(arg_rest, 1);
            have_assignment = true;
        } else if(argv && i) {
            if(++(*i) >= argc) {
                if(opt->id != ARG_BOOL && opt->fallback) THROW("nothing left to parse, expecting %.*s for '%.*s'", RSTR_F(argopt_list(opt)), RSTR_F(opt->str)); // TODO: MAYBE SAY WHAT WE EXPECT
            } else {
                observe123 = RSTR_L(argv[*i]);
            }
        } else {
            /* environmental variable */
            observe123 = arg_rest;
            have_assignment = true;
            //observe = RSTR("");
            //THROW("nothing left to parse, expecting %.*s for '%.*s'", RSTR_F(argopt_list(opt)), RSTR_F(opt->str));
        }
    }
    if(opt->id != ARG_STRING) {
        //printff("CHECKING FOR VECTOR: [%.*s]", RSTR_F(observe123));
        if(rstr_length(observe123) && rstr_get_front(&observe123) == '[') {
            if(opt->ninput == 1) {
                THROW("option '%.*s' %.*s is no vector, can't parse '%.*s'", RSTR_F(opt->str), RSTR_F(argopt_list(opt)), RSTR_F(observe123));
            }
            size_t i_pair = rstr_pair_ch(observe123, ']');
            if(i_pair >= rstr_length(observe123)) {
                THROW("could not find matching pair ']' in '%.*s'", RSTR_F(observe123));
            }
            have_vector = true;
            observe123 = RSTR_I0(RSTR_IE(observe123, i_pair), 1);
            //printff("FOUND a vector, pair: %zu / %zu: '%.*s'", i_pair, rstr_length(observe), RSTR_F(observe));
        } else if(rstr_find_ch(observe123, ',', 0) < rstr_length(observe123)) {
            have_vector = true;
        } else {
            observe = observe123;
        }
    } else {
        observe = observe123;
    }
    for(;;) {
        if(have_vector) {
            observe = rstr_splice(observe123, &observe, ',');
            //printff("VEC OBSERVE [%.*s]", RSTR_F(observe));
            if(!rstr_length(observe)) break;
        }
        //printff("%u/%u:::OBSERVING [%.*s] into %.*s", run+1,2, RSTR_F(observe), RSTR_F(argopt_list(opt)));
        /* now parse the string */
        if(true) {
            switch(opt->id) {
                case ARG_INT: {
                    if(!rstr_length(observe)) THROW("string is too short");
                    ssize_t z = strtoll(rstr_iter_begin(observe), &endptr, 0);
                    // TODO: check bounds (min, max) + conversion range error
                    if(endptr != rstr_iter_end(&observe)) THROW("cannot convert to number / wrong number format");
                    TRYC(argopt_assign_value(run, opt, &(ArgVal){.z = z}));
                } break;
                case ARG_FLOAT: {
                    if(!rstr_length(observe)) THROW("string is too short");
                    float f = strtod(rstr_iter_begin(observe), &endptr);
                    // TODO: check bounds (min, max) + conversion range error
                    if(endptr != rstr_iter_end(&observe)) THROW("cannot convert to number / wrong number format");
                    TRYC(argopt_assign_value(run, opt, &(ArgVal){.f = f}));
                } break;
                case ARG_STRING: {
                    TRYC(argopt_assign_value(run, opt, &(ArgVal){.s = observe}));
                } break;
                case ARG_BOOL: {
                    bool is_false = false;
                    bool is_true = false;
                    for(size_t i = 0; i < SIZE_ARRAY(check_true); ++i) {
                        is_true |= (!rstr_cmp_ci(&observe, &check_true[i]));
                    }
                    for(size_t i = 0; i < SIZE_ARRAY(check_false); ++i) {
                        is_false |= (!rstr_cmp_ci(&observe, &check_false[i]));
                    }
                    ASSERT(!(is_true && is_false), ERR_UNREACHABLE);
                    if(!is_true && !is_false) {
                        if(have_assignment) {
                            THROW("invalid '%.*s' %.*s provided: '%.*s'", RSTR_F(opt->str), RSTR_F(argopt_list(opt)), RSTR_F(observe));
                        }
                        if(opt->fallback) {
                            is_false = opt->fallback->b;
                            is_true = !opt->fallback->b;
                        } else {
                            is_true = true;
                        }
                        if(i) --(*i);
                    }
                    TRYC(argopt_assign_value(run, opt, &(ArgVal){.b = is_true}));
                } break;
                case ARG_OPTION: {
                    if(!opt->options) THROW(ERR_UNREACHABLE);

                    size_t i_split = rstr_find_ch(observe, '=', 0);
                    RStr search = RSTR_IE(observe, i_split);
                    RStr rest = RSTR_I0(observe, i_split);

                    ArgOpt *got = targopt_get(&opt->options->table, &search);
                    if(!got) THROW("did not find any matching option in '%.*s' %.*s : %.*s", RSTR_F(opt->str), RSTR_F(argopt_list(opt)), RSTR_F(search));
                    TRYC(arg_parse_val(arg, argc, argv, got, rest, i, deep_error, run));
                    TRYC(argopt_assign_value(run, opt, &(ArgVal){.z = got->index}));

                } break;
                case ARG_NONE: {
                } break;
                default: break;
            }
        } 
        if(run) {
            if(opt->action.function) {
                //printff("CALLING FUNCTION %p from %.*s", opt->action.function, RSTR_F(opt->str));
                opt->action.function(opt->action.data);
            }
            ++opt->consumed;
        }
        if(!have_vector) break;
    }
    return 0;
error:
    if(endptr && rstr_length(observe)) {
        if(endptr >= rstr_iter_begin(observe)) {
            int i_split = (int)(endptr - rstr_iter_begin(observe));
            ERR_PRINTF("%8sparsing " F("'", FG_RD_B BOLD) F("%.*s", FG_GN_B BOLD) F("%.*s'", FG_RD_B BOLD) "\n", "", RSTR_F(RSTR_IE(observe, i_split)), RSTR_F(RSTR_I0(observe, i_split)));
            ERR_PRINTF("%8s        "   " "                F("%*s^", FG_RD_B BOLD) "\n", "", i_split, "");
        } else {
            ERR_PRINTF("%8sparsing " F("'%.*s'", FG_RD_B BOLD) "\n", "", RSTR_F(observe));
        }
    }
    if(*deep_error) return -1;
    if(opt->id == ARG_BOOL) {
        ERR_PRINTF("%8svalid " F("'TRUE'", FG_BL_B) " values (case insensitive)\n", "");
        for(size_t i = 0; i < SIZE_ARRAY(check_true); ++i) {
            ERR_PRINTF(F("%8s%s", FG_CY_B) "" F("%.*s", FG_BL_B BOLD) "\n", "", i + 1 < SIZE_ARRAY(check_true) ? (!i ? ARG_ART_TOP : ARG_ART_TSHAPE) : ARG_ART_BOT, RSTR_F(check_true[i]));
        }
        ERR_PRINTF("%8svalid " F("'FALSE'", FG_BL_B) " values (case insensitive)\n", "");
        for(size_t i = 0; i < SIZE_ARRAY(check_false); ++i) {
            ERR_PRINTF(F("%8s%s", FG_CY_B) "" F("%.*s", FG_BL_B BOLD) "\n", "", i + 1 < SIZE_ARRAY(check_false) ? (!i ? ARG_ART_TOP : ARG_ART_TSHAPE) : ARG_ART_BOT, RSTR_F(check_false[i]));
        }
    }
    if(opt->id == ARG_OPTION) {
        if(!opt->options) THROW_PRINT(ERR_UNREACHABLE); // TODO: make this actually unreachable vai arg_attach ...
        ERR_PRINTF("%8svalid " F("'%.*s'", FG_BL_B BOLD) " values (any of %zu case sensitive)\n", "", RSTR_F(opt->str), vpargopt_length(opt->options->array));
        for(size_t i = 0; i < vpargopt_length(opt->options->array); ++i) {
            ArgOpt *option = vpargopt_get_at(&opt->options->array, i);
            ERR_PRINTF(F("%8s%s", FG_CY_B) "" F("%.*s", FG_BL_B BOLD) "\n", "", i + 1 < vpargopt_length(opt->options->array) ? (!i ? ARG_ART_TOP : ARG_ART_TSHAPE) : ARG_ART_BOT, RSTR_F(option->str));
        }
    }
    *deep_error = true;
    return -1;
} /*}}}*/

ErrImpl arg_defaults(ArgObj *obj) { /*{{{*/
    ASSERT_ARG(obj);
    for(size_t i = 0; i < vpargopt_length(obj->array); ++i) {
        ArgOpt *opt = vpargopt_get_at(&obj->array, i);
        if(!opt->fallback) continue;
        if(opt->id == ARG_OPTION) {
            TRYC(arg_defaults(opt->options));
        } else {
            TRYC(argopt_assign_value(false, opt, opt->fallback));
            //memcpy(opt->value, &opt->fallback.val, sizeof(*opt->value));
        }
    }
    return 0;
error:
    return -1;
} /*}}}*/

ErrDecl arg_verify_rec(Arg *arg, ArgObj *obj, bool option_has_optional_data) { /*{{{*/
    ASSERT_ARG(arg);
    if(!obj) return 0;
    for(size_t i = 0; i < vpargopt_length(obj->array); ++i) {
        ArgOpt *opt = vpargopt_get_at(&obj->array, i);
        if(opt->id == ARG_OPTION) {
            TRYC(arg_verify_rec(arg, opt->options, (bool)opt->value));
        } else if(!option_has_optional_data) {
            if(opt->id == ARG_NONE && !opt->action.function) THROW("option '%.*s' (id %.*s) has no function to call!", RSTR_F(opt->str), RSTR_F(argopt_list(opt)));
            else if(!opt->value && !opt->action.function) THROW("option '%.*s' (id %.*s) gets set no value and is no function is called!", RSTR_F(opt->str), RSTR_F(argopt_list(opt)));
        }
    }
    return 0;
error:
    return -1;
} /*}}}*/

/* TODO: 
 * - verify environmental variables ? special rules ??
 * */
ErrDecl arg_verify(Arg *arg) { /*{{{*/
    /* TODO: CHECK FOR NO-SPACES IN ARGUMENTS ... */
    /* verify positional arguments */
    if(!vpargopt_length(arg->positional.array) && !vpargopt_length(arg->options.array)) THROW("there are neither positional nor options to verify!");
    ArgOpt *opt_prev = 0, *opt = 0;
    for(size_t i = 0; i < vpargopt_length(arg->positional.array); ++i) {
        opt_prev = opt;
        opt = vpargopt_get_at(&arg->positional.array, i);
        if(opt->c) THROW("positional '%.*s' don't need a character '%c'", RSTR_F(opt->str), opt->c);
        if(opt_prev && opt_prev->fallback && !opt->fallback) THROW("optional positional should be last '%.*s' .. '%.*s'", RSTR_F(opt_prev->str), RSTR_F(opt->str));
        if(opt->id == ARG_OPTION) {
            TRYC(arg_verify_rec(arg, opt->options, (bool)opt->value));
        }
    }
    /* verify options */
    TRYC(arg_verify_rec(arg, &arg->options, false));
    return 0;
error:
    return -1;
} /*}}}*/

#define ERR_arg_parse_run(a,b,c,run) "failed parsing pass %u", run
ErrDecl arg_parse_run(Arg *arg, size_t argc, const char **argv, bool run) { /*{{{*/
    ASSERT_ARG(arg);
    bool rest_is_positional = false;
    //(void)arg_help(arg);
    for(size_t i = 1; i < argc; ++i) {
        if(arg->exit_early) break;
        /* observe */
        RStr arg_base = RSTR_L(argv[i]);
        bool is_positional = false;
        bool is_long = false;
        bool is_short = false;
        if(!rest_is_positional) {
            if(rstr_length(arg_base) == 2 && !rstr_cmp(&arg_base, &RSTR("--"))) {
                rest_is_positional = true;
                continue;
            } else if(rstr_length(arg_base) >= 2) {
                if(!rstr_cmp(&RSTR_IE(arg_base, 2), &RSTR("--"))) {
                    is_long = true;
                } else if(rstr_get_front(&arg_base) == '-') {
                    is_short = true;
                } else {
                    is_positional = true;
                }
            }
        }
        //printff("pos %u, long %u, short %u, rest %u", is_positional, is_long, is_short, rest_is_positional);
        /* action */
        //printff("%zu <== i", i);
        if(is_positional || rest_is_positional) {
            //printff(">>IS POSITIONAL");
            ArgOpt *opt = targopt_get(&arg->positional.table, &arg_base);
            if(opt) {
                TRYC(arg_parse_val(arg, argc, argv, opt, RSTR(""), &i, 0, run));
            } else if(arg->positions_used[run] < vpargopt_length(arg->positional.array)) {
                --i; /* not really clean, but the arg_parse_val consumes the first one regardless, assuming it's an option. here it's not */
                opt = vpargopt_get_at(&arg->positional.array, arg->positions_used[run]++);
                TRYC(arg_parse_val(arg, argc, argv, opt, RSTR(""), &i, 0, run));
            } else if(run && arg->rest.allow) {
                TRYG(vrstr_push_back(&arg->rest.vrstr, &arg_base));
            } //THROW("TODO:???");
        } else if(is_long) {
            //printff(">>IS LONG: '%.*s'", RSTR_F(arg_base));
            size_t i_split = rstr_find_ch(arg_base, '=', 0);
            RStr arg_search = RSTR_IE(arg_base, i_split);
            RStr arg_rest = RSTR_I0(arg_base, i_split);
            ArgOpt *opt = targopt_get(&arg->options.table, &arg_search);
            if(opt) {
                TRYC(arg_parse_val(arg, argc, argv, opt, arg_rest, &i, 0, run));
            } else {
                THROW("INVALID OPTION: '%.*s'", RSTR_F(arg_search));
            }
        } else if(is_short) {
            //printff(">>IS SHORT: '%.*s'", RSTR_F(arg_base));
            RStr arg_observe = RSTR_I0(arg_base, 1);
            while(rstr_length(arg_observe)) {
                unsigned char c = rstr_get_front(&arg_observe);
                TArgOptItem *item = arg->cs[c];
                if(!item) THROW("INVALID ARGUMENT: '%c'", c);
                ASSERT(item->val, ERR_UNREACHABLE);
                TRYC(arg_parse_val(arg, argc, argv, item->val, RSTR(""), &i, 0, run));
                /* done; next */
                ++arg_observe.first;
            }
        }
    }
    //printff("run %i/%i, exit? %u", run+1, 2, arg->exit_pre);
    if(run && !arg->exit_early && arg->positions_used[run] < vpargopt_length(arg->positional.array)) {
        ArgOpt *opt = vpargopt_get_at(&arg->positional.array, arg->positions_used[run]);
        if(!opt->fallback) THROW("too few (%zu) required arguments passed", arg->positions_used[run]);
    }
    return 0;
error:
    return -1;
}; /*}}}*/

/* TODO:
 * - if last argument of whatever is of STRING ... then collect rest of arguments ..
 *   e.g. ./search.out [option=]search a string that isn't terminated
 * */
ErrImpl arg_parse(Arg *arg, size_t argc, const char **argv) { /*{{{*/
    ASSERT(argc > 0, "can't have 0 arguments");
    arg->name = RSTR_L(argv[0]);

    TRYC(arg_defaults(&arg->environment));
    for(size_t i = 0; i < vpargopt_length(arg->environment.array); ++i) {
        ArgOpt *var = vpargopt_get_at(&arg->environment.array, i);
        RStr rest = RSTR_L(getenv(rstr_iter_begin(var->str)));
        //printff("'%.*s' = '%.*s'", RSTR_F(var->str), RSTR_F(rest));
        TRYC(arg_parse_val(arg, 0, 0, var, rest, 0, 0, false));
        TRYC(arg_parse_val(arg, 0, 0, var, rest, 0, 0, true));
    }

    TRYC(arg_verify(arg));
    TRYC(arg_defaults(&arg->positional));
    TRYC(arg_defaults(&arg->options));
    TRYC(arg_parse_run(arg, argc, argv, false));
    TRYC(arg_parse_run(arg, argc, argv, true));

    return 0;
error:
    return -1;
} /*}}}*/

void arg_allow_rest(Arg *arg, RStr info) {
    ASSERT_ARG(arg);
    arg->rest.allow = true;
    arg->rest.info = info;
}

ErrImpl arg_attach_help(Arg *arg, RStr arg_info, RStr arg_url) { /*{{{*/
    ASSERT_ARG(arg);
    arg->info = arg_info;
    arg->url = arg_url;
    ArgOpt opt = {0};
    opt.c = 'h';
    opt.str = RSTR("--help");
    opt.help = RSTR("display this help information");
    opt.action.function = arg_help;
    opt.action.data = arg;
    opt.quit = true;
    TRYC(arg_attach(&arg->options, &opt, arg));
    return 0;
error:
    return -1;
} /*}}}*/

ErrDecl arg_attach_version(Arg *arg, unsigned char c, RStr str) { /*{{{*/
    ASSERT_ARG(arg);
    arg_anon(argopt_new(arg, &arg->options, c, str, RSTR("print version and exit")), true, arg_version, arg);
    return 0;
} /*}}}*/

ErrImpl arg_positional(ArgObj *obj, ArgOpt *opt) { /*{{{*/
    ASSERT_ARG(opt);
    if(!rstr_length(opt->str)) THROW("positional has no display name (e.g. threads)");
    if(!rstr_length(opt->help)) THROW("positional has no description");
    if(opt->id == ARG_NONE) THROW("cannot add no argument of type %.*s", RSTR_F(argopt_list(opt)));
    if(opt->ninput != 1) THROW("cannot take %zu values in positional argument", opt->ninput);
    /* if fallback.use == true : the values is optional */
    if(opt->c) THROW(ERR_UNREACHABLE);
    // TODO: cross-check that there is nothing of equal name in arg_attach!!!
    if(!opt->fallback && vpargopt_length(obj->array)) {
        ArgOpt *back = vpargopt_get_back(&obj->array);
        if(back->fallback) THROW("optional argument must be after required ones");
    }
    TArgOptItem *once = targopt_once(&obj->table, &opt->str, opt);
    TRYG(vpargopt_push_back(&obj->array, once->val));
    if(!once) THROW("positional already exists");
    return 0;
error:
    return -1;
} /*}}}*/

// TODO: CHECK so we don't add true, false, etc.
ErrDecl arg_attach(ArgObj *obj, ArgOpt *opt, Arg *arg_if_c_is_set) { /*{{{*/
    ASSERT_ARG(obj);
    ASSERT_ARG(opt);
    if(!rstr_length(opt->str)) THROW("argument has no long option (e.g. --help)");
    if(!rstr_length(opt->help)) THROW("argument has no description");
    if(rstr_find_ws(opt->str) < rstr_length(opt->str)) THROW("cannot have whitespace in argument");
    if(rstr_length(opt->str) >= 2 && rstr_get_at(&opt->str, 0) == '-' && rstr_get_at(&opt->str, 0) != '-') THROW("argument begins with '-' but is not followed up with a second '-'");
    if(opt->id == ARG_NONE && !opt->action.function) THROW("type is NONE but no function provided");
    /* add */
    // TODO: cross-check that there is nothing of equal name in arg_positional!!!
    TArgOptItem *once = targopt_once(&obj->table, &opt->str, opt);
    if(!once) THROW("option '%.*s' already exists", RSTR_F(opt->str));
    TRYG(vpargopt_push_back(&obj->array, once->val));
    unsigned char c = opt->c;
    if(c) {
        ASSERT_ARG(arg_if_c_is_set);
        if(arg_if_c_is_set->cs[c]) THROW("short argument '%c' already exists (set to '%.*s')", c, RSTR_F(arg_if_c_is_set->cs[c]->val->str));
        arg_if_c_is_set->cs[c] = once;
    }
    return 0;
error:
    ERR_PRINTF("%8sfailed attaching argument '%.*s'\n", "", RSTR_F(opt->str));
    return -1;
} /*}}}*/

void arg_free(Arg *arg) { /*{{{*/
    ASSERT_ARG(arg);
    targopt_free(&arg->options.table);
    vpargopt_free(&arg->options.array);
    targopt_free(&arg->positional.table);
    vpargopt_free(&arg->positional.array);
    targopt_free(&arg->environment.table);
    vpargopt_free(&arg->environment.array);
    vrstr_free(&arg->rest.vrstr);
} /*}}}*/

ArgOpt *argopt_new(Arg *arg, ArgObj *obj, unsigned char c, RStr str, RStr help) { /*{{{*/
    ASSERT_ARG(arg);
    ASSERT_ARG(obj);

    if(!rstr_length(str)) THROW("argument has no long option (e.g. --help)");
    if(!rstr_length(help)) THROW("argument has no description");
    if(rstr_find_ws(str) < rstr_length(str)) THROW("cannot have spaces in option: '%.*s'", RSTR_F(str));
    if(rstr_find_ws(str) < rstr_length(str)) THROW("cannot have whitespace in argument");
    if(rstr_length(str) >= 2 && rstr_get_at(&str, 0) == '-' && rstr_get_at(&str, 0) != '-') THROW("argument begins with '-' but is not followed up with a second '-'");
    if(obj == &arg->options) {
        if(rstr_length(str) < 2 || (rstr_length(str) > 2 && rstr_cmp(&RSTR_IE(str, 2), &RSTR("--")))) {
            THROW("required for options of first level to start with '--'; have '%.*s'", RSTR_F(str));
        }
    }

    /* add */
    // TODO: cross-check that there is nothing of equal name in arg_positional!!!
    TArgOptItem *once = targopt_once(&obj->table, &str, &(ArgOpt){.c = c, .str = str, .help = help});
    if(!once) THROW("option '%.*s' already exists", RSTR_F(str));
    once->val->index = vpargopt_length(obj->array);
    TRYG(vpargopt_push_back(&obj->array, once->val));
    if(c) {
        if(arg->cs[c]) THROW("short argument '%c' already exists (set to '%.*s')", c, RSTR_F(arg->cs[c]->val->str));
        arg->cs[c] = once;
    }

    return once->val;
error:
    return 0;
} /*}}}*/

#if 0
ArgVar *argvar_new(Arg *arg, RStr str, RStr help) { /*{{{*/
    ASSERT_ARG(arg);
    ArgEnv *env = &arg->environment;
    if(!rstr_length(str)) THROW("variable has no long option (e.g. --help)");
    if(!rstr_length(help)) THROW("variable has no description");
    TArgVarItem *once = targvar_once(&env->table, &str, &(ArgVar){.str = str, .help = help});
    if(!once) THROW("variable '%.*s' already exists", RSTR_F(str));
    TRYG(vpargvar_push_back(&env->array, once->val));
    return once->val;
error:
    return 0;
} /*}}}*/
#endif

/* functions to actually configure the options {{{ */

#define ARG_BASE() do { \
        if(value) opt->value = (void *)value; \
        /*opt->fallback.use = (bool)fallback;*/ \
        if(fallback) opt->fallback = (void *)fallback; \
        opt->action.data = data; \
        opt->action.function = function; \
    } while(0)

#define ARG_MINMAX(accessor, default_min, default_max) do { \
        if(min < max) { \
            opt->min.accessor = min; \
            opt->max.accessor = max; \
        } else { \
            opt->min.accessor = default_min; \
            opt->max.accessor = default_max; \
        } \
    } while(0)

ArgOpt *arg_option(ArgOpt *opt, ssize_t *value) {
    ASSERT_ARG(opt);
    opt->id = ARG_OPTION;
    opt->options = malloc(sizeof(*opt->options));
    opt->value = (void *)value;
    memset(opt->options, 0, sizeof(*opt->options));
    opt->ninput = 1;
    return opt;
}

ArgOpt *arg_voption(ArgOpt *opt, VSize *value, size_t ninput) {
    ASSERT_ARG(opt);
    opt->id = ARG_OPTION;
    opt->options = malloc(sizeof(*opt->options));
    opt->value = (void *)value;
    memset(opt->options, 0, sizeof(*opt->options));
    opt->ninput = 0;
    return opt;
}

#if 0
void argenv_str    (ArgVar *var, RStr *value, RStr *fallback) {
    ASSERT_ARG(var);
    var->value = (void *)value;
    var->fallback = (void *)fallback;
    var->id = ARG_STRING;
}
#endif

void arg_int   (ArgOpt *opt, ssize_t *value, ssize_t *fallback, bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max) {
    ASSERT_ARG(opt);
    ARG_BASE();
    ARG_MINMAX(z, SSIZE_MIN, SSIZE_MAX);
    opt->id = ARG_INT;
    opt->ninput = 1;
}

void arg_vint  (ArgOpt *opt, VSize   *value, VSize *fallback,   bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max, size_t ninput) {
    ASSERT_ARG(opt);
    ASSERT(ninput != 1, "use non-vector version if limit is one value");
    ARG_BASE();
    ARG_MINMAX(z, SSIZE_MIN, SSIZE_MAX);
    opt->id = ARG_INT;
    opt->ninput = ninput;
}

void arg_float (ArgOpt *opt, double  *value, double *fallback,  bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max) {
    ASSERT_ARG(opt);
    ARG_BASE();
    ARG_MINMAX(f, 0, 0);
    opt->id = ARG_FLOAT;
    opt->ninput = 1;
}

void arg_vfloat(ArgOpt *opt, VDouble *value, VDouble *fallback, bool quit, ArgFunc function, void *data, ssize_t min, ssize_t max, size_t ninput) {
    ASSERT_ARG(opt);
    ASSERT(ninput != 1, "use non-vector version if limit is one value");
    ARG_BASE();
    ARG_MINMAX(f, 0, 0);
    opt->id = ARG_FLOAT;
    opt->ninput = ninput;
}

void arg_str   (ArgOpt *opt, RStr    *value, RStr *fallback,    bool quit, ArgFunc function, void *data) {
    ASSERT_ARG(opt);
    ARG_BASE();
    opt->id = ARG_STRING;
    opt->ninput = 1;
}

void arg_vstr  (ArgOpt *opt, VrStr   *value, VrStr *fallback,   bool quit, ArgFunc function, void *data, size_t ninput) {
    ASSERT_ARG(opt);
    ASSERT(ninput != 1, "use non-vector version if limit is one value");
    ARG_BASE();
    opt->id = ARG_STRING;
    opt->ninput = ninput;
}

void arg_bool  (ArgOpt *opt, bool    *value, bool *fallback,    bool quit, ArgFunc function, void *data) {
    ASSERT_ARG(opt);
    ARG_BASE();
    opt->id = ARG_BOOL;
    opt->ninput = 1;
}

void arg_vbool (ArgOpt *opt, VBool   *value, VBool *fallback,   bool quit, ArgFunc function, void *data, size_t ninput) {
    ASSERT_ARG(opt);
    ASSERT(ninput != 1, "use non-vector version if limit is one value");
    ARG_BASE();
    opt->id = ARG_BOOL;
    opt->ninput = ninput;
}

void arg_anon  (ArgOpt *opt, bool quit, ArgFunc function, void *data) {
    ASSERT_ARG(opt);
    opt->action.function = function;
    opt->action.data = data;
    opt->id = ARG_NONE;
    opt->ninput = 1;
}

void arg_enum (ArgOpt *opt, bool quit, ArgFunc function, void *data, size_t val) {
    ASSERT_ARG(opt);
    opt->action.function = function;
    opt->action.data = data;
    opt->id = ARG_NONE;
    opt->index = val;
    opt->ninput = 1;
}

/* }}} */

