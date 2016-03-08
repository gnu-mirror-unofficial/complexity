
/*
 *  This file is part of Complexity.
 *  Complexity Copyright (c) 2011-2016 by Bruce Korb - all rights reserved
 *
 *  Complexity is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Complexity is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "opts.h"
#include <setjmp.h>

static jmp_buf bail_on_proc;

static char const err_fmt[]    = "error: %s %s\n";
static char const line_score[] = "line %5d score %5u\n";

typedef score_t (handler_func_t)(state_t *);

/**
 * subexpression types that have been seen.  Tracking is only done within
 * one parenthesized expression.  If two or more types are mixed, the
 * expression gets penalized.  If there is ever an assignment within
 * an expression, the expression is penalized.
 */
typedef struct {
    int saw_and, saw_or, saw_assign, saw_relop;
    score_t res;
} subexpr_seen_t;

/*
 * declare the dispatch functions and define the table.
 */
#define handle_TKN_KW_SWITCH    handle_TKN_KW_FOR
#define handle_TKN_KW_WHILE     handle_TKN_KW_FOR
#define handle_TKN_KW_ELSE      handle_noop
#define handle_TKN_KW_EXTERN    handle_invalid
#define handle_TKN_KW_GOTO      handle_invalid
#define handle_TKN_EMPTY        handle_invalid
#define handle_TKN_EOF          handle_invalid
#define handle_TKN_NUMBER       handle_expression
#define handle_TKN_NAME         handle_expression
#define handle_TKN_REL_OP       handle_invalid
#define handle_TKN_ARITH_OP     handle_expression
#define handle_TKN_LOGIC_AND    handle_invalid
#define handle_TKN_LOGIC_OR     handle_invalid
#define handle_TKN_ASSIGN       handle_invalid
#define handle_TKN_ELLIPSIS     handle_invalid
#define handle_TKN_LIT_CLSPAREN handle_invalid
#define handle_TKN_LIT_COMMA    handle_noop
#define handle_TKN_LIT_COLON    handle_invalid
#define handle_TKN_LIT_QUESTION handle_expression
#define handle_TKN_LIT_CLSBRACK handle_invalid
#define handle_TKN_LIT_OPNBRACK handle_array_init
#define handle_TKN_LIT_CBRACE   handle_invalid
#define handle_TKN_LIT_OBRACE   handle_stmt_block
#define handle_TKN_LIT_OPNPAREN handle_p_expr

#define _Ttbl_(_v, _n) handle_ ## _n,
static handler_func_t TOKEN_TABLE
    handle_parms, handle_bracket_expr;
#undef  _Ttbl_

static handler_func_t * score_fun[TOKEN_MAX] = {
#define _Ttbl_(_v, _n) [_v] = handle_ ## _n,
    TOKEN_TABLE
#undef  _Ttbl_
};

#define APPLY_NEST_PENALTY(_s)   ((_s) * penalty)

static int statement_depth = 0;

static score_t
handle_subexpr(state_t * sc, bool is_for_clause);

static score_t
bad_token(state_t * sc, char const * where, token_t ev)
{
    static char const msgfmt[] =
        "error on line %d of %s in file %s(%d):\n"
        "in context %s, token %s (%d) is invalid.\n";
    static char const * const tkn_names[] = {
#define _Ttbl_(_v, _n) [_v] = #_n,
        TOKEN_TABLE
#undef  _Ttbl_
    };

    char const * p = (ev < TOKEN_MAX) ? tkn_names[ev] : NULL;
    char invbuf[32];

    if (p == NULL) {
        sprintf(invbuf, "%d is not a valid token", ev);
        p = invbuf;
    }

    fprintf(stderr, msgfmt, sc->st_line_ct, sc->pname, sc->st_fstate->fs_fname,
            sc->proc_line + sc->st_line_ct, where, p, ev);
    return MAX_SCORE;
}

static token_t
next_score_token(state_t * sc)
{
    fstate_t * fs = sc->st_fstate;
    token_t    tk = next_token(fs);

    if ((tk == TKN_EOF) || (fs->fs_scan > sc->st_end))
        longjmp(bail_on_proc, 1);

    if (tk != TKN_KW_GOTO)
        return tk;
    sc->goto_ct++;
    return TKN_NAME;
}

static void
unget_score_token(state_t * sc)
{
    unget_token(sc->st_fstate);
}

/**
 * The open brace for the statement block has already been processed.
 */
static score_t
handle_stmt_block(state_t * sc)
{
    fstate_t * fs = sc->st_fstate;
    score_t res = 0;

    /*
     * Bookwork for the first call for a procedure
     */
    token_t ev = next_score_token(sc);
    if (sc->st_nc_line_ct < 0) {
        sc->st_line_ct    = fs->cur_line;
        sc->st_nc_line_ct = fs->nc_line;
    }

    if (++statement_depth > sc->st_depth_warned)
        sc->st_depth_warned = statement_depth;

    for (;; ev = next_score_token(sc)) {
        switch (ev) {
        case TKN_LIT_CBRACE:
            if (HAVE_OPT(TRACE))
                fprintf(trace_fp, line_score,
                        fs->cur_line, (unsigned int)res);
            statement_depth--;
            return (res > MAX_SCORE) ? MAX_SCORE : res;

        default:
            res += score_fun[ev](sc);
        }
    }
}

static score_t
handle_invalid(state_t * sc)
{
    sc->st_fstate->fs_scan = sc->st_end;
    fprintf(stderr, "invalid transition\n");
    return MAX_SCORE;
}

static score_t
handle_noop(state_t * sc)
{
    return 0;
}

static char const *
fiddle_subexpr_score(subexpr_seen_t * ses)
{
    int which =
        (ses->saw_and    ? 0x01 : 0) +
        (ses->saw_or     ? 0x02 : 0) +
        (ses->saw_assign ? 0x04 : 0) +
        (ses->saw_relop  ? 0x08 : 0);
    int tmp;

    switch (which) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x08:
        return NULL;

    case 0x04:
        ses->res += penalty * (score_t)ses->saw_assign;
        return "assignment within expression";

    case 0x07:
        ses->res += penalty * (score_t)ses->saw_assign;
        /* FALLTHROUGH */

    case 0x03:
        tmp = (ses->saw_and + 1) * ses->saw_or;
        ses->res += penalty * (score_t)tmp;
        return "AND and OR expressions";

    case 0x05:
    case 0x06:
        ses->res += penalty * (score_t)ses->saw_assign;
        ses->res += (score_t)(ses->saw_and + ses->saw_or);
        return "assignments and boolean operators";

    case 0x09:
    case 0x0A:
        ses->res += subexp_penalty * (score_t)ses->saw_relop;
        return "comparison and boolean operators";

    case 0x0B:
        tmp = (ses->saw_and + 1) * ses->saw_or;
        ses->res += subexp_penalty * (score_t)(ses->saw_relop * tmp);
        return "AND, OR and comparison operators";

    case 0x0C:
        ses->res += penalty * (score_t)ses->saw_assign;
        ses->res += subexp_penalty * (score_t)ses->saw_relop;
        return "assignments and comparison operators";

    case 0x0D:
    case 0x0E:
        ses->res += penalty * (score_t)(
            ses->saw_assign + ses->saw_relop + ses->saw_and + ses->saw_or);
        return "many kinds of operators";

    case 0x0F:
        tmp = (ses->saw_and + 1) * ses->saw_or;
        ses->res += penalty * (score_t)(
            ses->saw_assign + ses->saw_relop + tmp);
        return "*ALL* kinds of operators";

    default:
        CX_ASSERT(which == 0);
    }
}

static score_t
handle_TKN_LIT_SEMI(state_t * sc)
{
    return 1;
}

static score_t
handle_subexpr(state_t * sc, bool is_for_clause)
{
    bool saw_name = false;
    int  tkn_ct   =  0;
    subexpr_seen_t ses = { 0, 0, 0, 0, 1.0 };
    int const start_nc_ln_ct = sc->st_fstate->nc_line;

    for (;;) {
        token_t ev = next_score_token(sc);
        tkn_ct++;

        switch (ev) {
        case TKN_LIT_CLSPAREN:
            if (tkn_ct <= 2) // name + close paren
                return 0;

            if (! is_for_clause) {
                char const * msg = fiddle_subexpr_score(&ses);
                if ((msg != NULL) && HAVE_OPT(TRACE))
                    fprintf(trace_fp, "line %5d expression score adjusted due "
                            "to mix of %s\n", sc->st_fstate->cur_line, msg);
            }

            ses.res += (score_t)(sc->st_fstate->nc_line - start_nc_ln_ct);
            if (ses.res > 1)
                ses.res -= 1;
            if (HAVE_OPT(TRACE))
                fprintf(trace_fp, line_score,
                        sc->st_fstate->cur_line, (unsigned int)ses.res);
            return ses.res;

        case TKN_LIT_OPNPAREN:
            if (saw_name)
                ses.res += handle_parms(sc);
            else {
                ses.res += handle_subexpr(sc, false) * subexp_penalty;

                /*
                 * We have not actually seen a name, but it could have
                 * been a parenthesized expression that yields a pointer.
                 * That pointer can reference a procedure or an array of
                 * memory.  We lie a little bit.
                 */
                ev = TKN_NAME;
            }
            break;

        case TKN_ASSIGN:
            /*
             * Only worry over assignment operators in nested expressions
             */
            ses.saw_assign += (statement_depth > 1) ? 1 : 0;
            break;

        case TKN_LOGIC_AND:
            ses.saw_and++;
            break;

        case TKN_LOGIC_OR:
            ses.saw_or++;
            break;

        case TKN_REL_OP:
            ses.saw_relop++;
            break;

        case TKN_LIT_COMMA:
        case TKN_LIT_SEMI:
            ses.res += 1.0;
            break;

        case TKN_NAME:
        case TKN_NUMBER:
        case TKN_ARITH_OP:
        case TKN_LIT_COLON:
        case TKN_LIT_QUESTION:
            break;

        case TKN_LIT_OBRACE:
            ses.res += APPLY_NEST_PENALTY(handle_stmt_block(sc));
            break;

        case TKN_LIT_OPNBRACK:
            ses.res += handle_bracket_expr(sc);
            break;

        default:
            return bad_token(sc, "parenthesized expression", ev);
        }

        saw_name = (ev == TKN_NAME);
    }
}

/*
 * handle parameters.  They are basically free, unless they
 * get extended to multiple lines or include parenthesized
 * lists.
 */
static score_t
handle_parms(state_t * sc)
{
    score_t res = 0;
    int const start_nc_ln_ct = sc->st_fstate->nc_line;

    for (;;) {
        token_t ev = next_score_token(sc);
        switch (ev) {
        case TKN_LIT_CLSPAREN:
        {
            /*
             * Set to zero if we are on the same non-comment line count
             */
            int l_ct_delta = sc->st_fstate->nc_line - start_nc_ln_ct;
            return (score_t)l_ct_delta + res;
        }

        case TKN_LIT_OPNPAREN:
        {
            score_t s = handle_subexpr(sc, false);
            if (s > 0)
                res += s;
            break;
        }

        case TKN_LIT_OBRACE:
            res += APPLY_NEST_PENALTY(handle_stmt_block(sc));
            break;

        case TKN_LIT_OPNBRACK:
        {
            score_t s = handle_bracket_expr(sc);
            if (s > 1)
                res += s - 1;
            break;
        }

        default:
            (void)handle_expression(sc);
            break;
        }
    }
}

/**
 * expressions always count at least 1.  Additional points
 * are added for multiple lines and complexity and lack
 * of parentheses when there are operators of different
 * precedence.
 */
static score_t
handle_expression(state_t * sc)
{
    score_t res = 1;
    int const start_nc_ln_ct = sc->st_fstate->nc_line;
    bool paren_is_parms    = true;
    bool cbrace_needs_semi = false;

    for (;;) {
        token_t ltk = sc->st_fstate->last_tkn;
        token_t ev = next_score_token(sc);
        bool next_paren_is_parms    = false;
        bool next_cbrace_needs_semi = false;

        switch (ev) {
        case TKN_LIT_CBRACE:
            if (HAVE_OPT(TRACE))
                fprintf(trace_fp, line_score,
                        sc->st_fstate->cur_line, (unsigned int)res);
            /* FALLTHROUGH */
        case TKN_LIT_CLSBRACK:
        case TKN_LIT_CLSPAREN:
            unget_score_token(sc);
            goto done;

        case TKN_LIT_COMMA:
            res += 1;
            break;

        case TKN_LIT_SEMI:
            goto done;

        case TKN_LIT_OPNPAREN:
            if (paren_is_parms)
                res += handle_parms(sc) - 1; // calling name already counted

            else {
                res += handle_subexpr(sc, false);
                next_paren_is_parms = true;
            }
            break;

        case TKN_NAME:
            next_paren_is_parms = true;
            break;

        case TKN_LIT_OBRACE:
            res += APPLY_NEST_PENALTY(handle_stmt_block(sc));
            if (! cbrace_needs_semi) {
                /*
                 * Someone has mutilated C syntax and made a macro into
                 * a block structured command.  (Bad idea, but common....)
                 * To fix this, we lie to our caller and claim we saw a
                 * semi-colon.
                 */
                sc->st_fstate->last_tkn = TKN_LIT_SEMI;
                goto done;
            }
            break;

        case TKN_LIT_OPNBRACK:
            res += handle_bracket_expr(sc) - 1;
            break;

        case TKN_ASSIGN:
            next_cbrace_needs_semi = true;
            break;

        case TKN_LIT_COLON:
            /*
             * A name followed by a colon is a statement label, unless
             * we are in the middle of a ternary operation.
             */
            if (sc->st_colon_need > 0) {
                sc->st_colon_need--;
            } else if (ltk ==TKN_NAME)
                goto done;
            break;

        case TKN_LIT_QUESTION:
            sc->st_colon_need++;
            break;

        case TKN_NUMBER:
        case TKN_ARITH_OP:
        default:
            break;
        }

        paren_is_parms    = next_paren_is_parms;
        cbrace_needs_semi = next_cbrace_needs_semi;
    }

done:
    {
        /*
         * Set the minimum to the number of lines used by the expr
         * and limit the maximum to MAX_SCORE
         */
        score_t min = 1 + (sc->st_fstate->nc_line - start_nc_ln_ct);
        if (res < min)
            res = min;
        else if (res > MAX_SCORE)
            res = MAX_SCORE;
    }
    return res;
}

static score_t
handle_p_expr(state_t * sc)
{
    unget_score_token(sc);
    return handle_expression(sc);
}

/*
 *  Handle these:
 *
 *  case VALUE:
 *  case (expression):
 *  case (expr1) ... (expr2):
 */
static score_t
handle_TKN_KW_CASE(state_t * sc)
{
    score_t case_score  = 1;
    int ellipsis_ct = 0;

    for (;;) {
        token_t ev = next_score_token(sc);
        switch (ev) {
        case TKN_LIT_OPNPAREN:
        {
            score_t pscore = handle_subexpr(sc, false);
            if (pscore > 0)
                case_score += pscore - 1;
            break;
        }

        case TKN_ARITH_OP:
        case TKN_REL_OP:
        case TKN_NAME:
        case TKN_NUMBER:
            break;

        case TKN_LIT_COLON:
            return case_score + ellipsis_ct;

        case TKN_ELLIPSIS:
            if (ellipsis_ct > 0)
                return bad_token(sc, "'case' statement ellipsis", ev);
            ellipsis_ct = 2;
            break;

        default:
            return bad_token(sc, "'case' statement", ev);
        }
    }
}

static score_t
handle_TKN_KW_DEFAULT(state_t * sc)
{
    token_t ev = next_score_token(sc);
    if (ev != TKN_LIT_COLON)
        return bad_token(sc, "'default' missing colon", ev);
    return 1;
}

static score_t
handle_TKN_KW_DO(state_t * sc)
{
    score_t res = 1;
    token_t ev = next_score_token(sc);

    switch (ev) {
    case TKN_LIT_OBRACE:
        res += APPLY_NEST_PENALTY(handle_stmt_block(sc));
        break;

    case TKN_LIT_OPNPAREN:
        res += handle_p_expr(sc);
        break;

    case TKN_KW_IF:
    case TKN_KW_DO:
    case TKN_KW_FOR:
    case TKN_KW_SWITCH:
    case TKN_KW_WHILE:
        res += APPLY_NEST_PENALTY(score_fun[ev](sc));
        break;

    default:
        res += handle_expression(sc);
    }

    ev = next_score_token(sc);
    if (ev != TKN_KW_WHILE)
        return bad_token(sc, "'do ...' missing 'while' ", ev);
    ev = next_score_token(sc);

    if (ev != TKN_LIT_OPNPAREN)
        return bad_token(sc, "while loop expression", ev);

    {
        int psc = APPLY_NEST_PENALTY(handle_subexpr(sc, false));
        if (psc < 1)
            psc = 1;
        res += psc;
    }
    ev = next_score_token(sc);

    if (ev != TKN_LIT_SEMI)
        return bad_token(sc, "do...while() missing semicolon", ev);
    return res;
}

static score_t
handle_TKN_KW_IF(state_t * sc)
{
    score_t res = 0;
    int     then_clause_done;
    token_t ev;

cascade_if:

    then_clause_done = 0;
    ev = next_score_token(sc);

    if (ev != TKN_LIT_OPNPAREN)
        return bad_token(sc, "if expression", ev);

    res += handle_subexpr(sc, false) + 1;
    ev   = next_score_token(sc);

do_if_clause:

    switch (ev) {
    case TKN_LIT_OBRACE:
        res += APPLY_NEST_PENALTY(handle_stmt_block(sc));
        break;

    case TKN_LIT_SEMI:
        break;

    case TKN_LIT_OPNPAREN:
        res += handle_subexpr(sc, false);
        goto simple_statement;

    case TKN_LIT_COMMA:
        ev = next_score_token(sc);
        goto do_if_clause;

    case TKN_LIT_OPNBRACK:
        res += handle_bracket_expr(sc) - 1;
        /* FALLTHROUGH */
    case TKN_ARITH_OP:
    case TKN_NAME:
    case TKN_NUMBER:
    simple_statement:
        res += handle_expression(sc);
        break;

    case TKN_KW_IF:
    case TKN_KW_DO:
    case TKN_KW_FOR:
    case TKN_KW_SWITCH:
    case TKN_KW_WHILE:
        res += APPLY_NEST_PENALTY(score_fun[ev](sc));
        break;

    default:
        return bad_token(sc, "bad if block", ev);
    }

    if (then_clause_done)
        return res;

    ev = sc->st_fstate->last_tkn;
    {
        token_t ntkn = next_score_token(sc);
        if (ntkn != TKN_KW_ELSE) {
            /*
             * Put the token back.  The caller will need it.
             */
            unget_score_token(sc);
            sc->st_fstate->last_tkn = ev;
            return res;
        }
    }

    ev = next_score_token(sc);
    if (ev == TKN_KW_IF)
        goto cascade_if;

    then_clause_done = 1;
    goto do_if_clause;
}

static score_t
handle_TKN_KW_FOR(state_t * sc)
{
    score_t res;
    bool    real_for = (sc->st_fstate->last_tkn == TKN_KW_FOR);
    token_t ev = next_score_token(sc);

    if (ev != TKN_LIT_OPNPAREN)
        return bad_token(sc, "loop expression", ev);

    res = handle_subexpr(sc, real_for);
    if (res < 1)
        res = 1;
    if (! real_for)
        res *= penalty;

    for (;;) {
        ev = next_score_token(sc);
        switch (ev) {
        case TKN_LIT_OBRACE:
            res += APPLY_NEST_PENALTY(handle_stmt_block(sc));
            return res;

        case TKN_LIT_SEMI:
            return res;

        case TKN_KW_IF:
        case TKN_KW_DO:
        case TKN_KW_FOR:
        case TKN_KW_SWITCH:
        case TKN_KW_WHILE:
            res += APPLY_NEST_PENALTY(score_fun[ev](sc));
            return res;

        case TKN_LIT_OPNPAREN:
            res += handle_subexpr(sc, false);
            continue;

        case TKN_LIT_OPNBRACK:
            res += handle_bracket_expr(sc) - 1;
            /* FALLTHROUGH */

        case TKN_NAME:
        case TKN_NUMBER:
            res += handle_expression(sc);
            if (sc->st_fstate->last_tkn == TKN_LIT_SEMI)
                return res;
            if (sc->st_fstate->last_tkn != TKN_LIT_COMMA)
                return bad_token(sc, "loop body ended badly",
                                 sc->st_fstate->last_tkn);
            /* FALLTHROUGH */

        default:
            continue;
        }
    }
}

static score_t
handle_bracket_expr(state_t * sc)
{
    score_t res = 0;

    for (;;) {
        res += handle_expression(sc);
        if (sc->st_fstate->last_tkn != TKN_LIT_COMMA)
            break;
        res -= 1;
    }

    token_t ev = next_score_token(sc);
    switch (ev) {
    case TKN_LIT_CLSBRACK:
        return (res > MAX_SCORE) ? MAX_SCORE : res;

    default:
        return bad_token(sc, "bracketed block", ev);
    }
}

static score_t
handle_array_init(state_t * sc)
{
    score_t res = handle_bracket_expr(sc) - 1;
    token_t ev  = next_score_token(sc);
    if (ev != TKN_ASSIGN)
        return bad_token(sc, "array element initializer", ev);
    return res + handle_expression(sc);
}

/**
 * return 'true' if the closing brace is on its own line.
 * This affects both the non-comment line count and
 * the total line count.
 */
static inline bool
check_own_line_close(state_t * sc)
{
    char const * p = sc->st_fstate->fs_scan - 1;
    CX_ASSERT(*p == '}');
    return IS_END_OF_LINE_CHAR(p[-1]);
}

/**
 * Score the procedure we just found.
 */
void
score_proc(state_t * score)
{
    static bool fix_dispatch = true;

    if (fix_dispatch) {
        int ix = 0;
        do  {
            if (score_fun[ix] == NULL)
                score_fun[ix] = handle_invalid;
        } while (++ix < TOKEN_MAX);
        fix_dispatch = false;
    }

    if (setjmp(bail_on_proc) != 0) {
        fprintf(stderr, "end of %s() in %s reached with open control blocks\n",
                score->pname, score->st_fstate->fs_fname);

        score->score = MAX_SCORE;
        return;
    }

    statement_depth = 0;
    score->st_line_ct    = \
        score->st_nc_line_ct = -1;

    score->score = handle_stmt_block(score);
    if (score->goto_ct > 0)
        score->score += score->goto_ct * scaling;

    if (score->st_depth_warned >= 5) {
        fprintf(stderr, "NOTE: proc %s in file %s line %u\n"
                "\tnesting depth reached level %u\n",
                score->pname, score->st_fstate->fs_fname,
                score->proc_line, score->st_depth_warned);
        if (score->st_depth_warned >= 7)
            fputs("==>\t*seriously consider rewriting the procedure*.\n", stderr);
    }

    if (score->st_fstate->fs_scan + 2 <= score->st_end) {
        fprintf(stderr, "procedure %s in %s ended before final close bracket\n",
                score->pname, score->st_fstate->fs_fname);

        score->score += penalty;
    }

    /*
     * No negative scores and no scores beyond the completely ridiculous
     * value of MAX_SCORE
     */
    if (score->score < 0)
        score->score = 0.0;

    else if (score->score < MAX_SCORE)
        score->score = (score_t)(unsigned int)((score->score * scaling) + 0.9);

    if (score->score > MAX_SCORE)
        score->score = MAX_SCORE;

    /*
     * Set the line counts for our score
     */
    bool close_on_own_line = check_own_line_close(score);
    int ct = 1 + (score->st_fstate->cur_line - score->st_line_ct) -
        (close_on_own_line ? 1 : 0);
    score->st_line_ct    = ct;
    ct = 1 + (score->st_fstate->nc_line  - score->st_nc_line_ct) -
        (close_on_own_line ? 1 : 0);
    score->st_nc_line_ct = ct;
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of score.c */
