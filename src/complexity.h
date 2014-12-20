
/*
 *  This file is part of Complexity.
 *  Complexity Copyright (c) 2011-2014 by Bruce Korb - all rights reserved
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

#ifndef COMPLEXITY_H_GUARD
#define COMPLEXITY_H_GUARD

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "char-types.h"
#include "opts.h"

#ifndef NUL
# define NUL '\0'
#endif

#ifndef NL
# define NL  '\n'
#endif

#define TOKEN_TABLE                     \
    _Ttbl_(  0, TKN_EMPTY)              \
    _Ttbl_(  1, TKN_EOF)                \
    _Ttbl_(  2, TKN_NAME)               \
    _Ttbl_(  3, TKN_NUMBER)             \
    _Ttbl_(  4, TKN_REL_OP)             \
    _Ttbl_(  5, TKN_ARITH_OP)           \
    _Ttbl_(  6, TKN_LOGIC_AND)          \
    _Ttbl_(  7, TKN_LOGIC_OR)           \
    _Ttbl_(  8, TKN_ASSIGN)             \
    _Ttbl_(  9, TKN_ELLIPSIS)           \
                                        \
    _Ttbl_( 11, TKN_KW_CASE)            \
    _Ttbl_( 12, TKN_KW_DEFAULT)         \
    _Ttbl_( 13, TKN_KW_DO)              \
    _Ttbl_( 14, TKN_KW_ELSE)            \
    _Ttbl_( 15, TKN_KW_FOR)             \
    _Ttbl_( 16, TKN_KW_GOTO)            \
    _Ttbl_( 17, TKN_KW_IF)              \
    _Ttbl_( 18, TKN_KW_SWITCH)          \
    _Ttbl_( 19, TKN_KW_WHILE)           \
    _Ttbl_( 20, TKN_KW_EXTERN)          \
                                        \
    _Ttbl_('(', TKN_LIT_OPNPAREN)       \
    _Ttbl_(')', TKN_LIT_CLSPAREN)       \
    _Ttbl_(',', TKN_LIT_COMMA)          \
    _Ttbl_(':', TKN_LIT_COLON)          \
    _Ttbl_(';', TKN_LIT_SEMI)           \
    _Ttbl_('[', TKN_LIT_OPNBRACK)       \
    _Ttbl_(']', TKN_LIT_CLSBRACK)       \
    _Ttbl_('{', TKN_LIT_OBRACE)         \
    _Ttbl_('}', TKN_LIT_CBRACE)

#define _Ttbl_(_v, _n) _n = _v,
typedef enum { TOKEN_TABLE TOKEN_MAX } token_t;
#undef  _Ttbl_

#define RES_WORD_TABLE \
    _Ktbl_(case,    TKN_KW_CASE)    \
    _Ktbl_(default, TKN_KW_DEFAULT) \
    _Ktbl_(do,      TKN_KW_DO)      \
    _Ktbl_(else,    TKN_KW_ELSE)    \
    _Ktbl_(extern,  TKN_KW_EXTERN)  \
    _Ktbl_(for,     TKN_KW_FOR)     \
    _Ktbl_(goto,    TKN_KW_GOTO)    \
    _Ktbl_(if,      TKN_KW_IF)      \
    _Ktbl_(switch,  TKN_KW_SWITCH)  \
    _Ktbl_(while,   TKN_KW_WHILE)

typedef struct {
    FILE *          fp;
    char const *    fname;
    char const *    text;
    char const *    scan;
    bool            bol;
    token_t         last_tkn;
    char const *    tkn_text;
    size_t          tkn_len;
    int             tkn_line;
    int             cur_line;
    int             nc_line;
} fstate_t;

typedef struct {
    int             ln_ct;
    int             ncln_ct;
    int             ln_st;
    int             ncln_st;
    score_t         score;
    int             goto_ct;
    int             proc_line;
    char *          end;
    fstate_t *      fstate;
    char            pname[256];
} state_t;

static inline void state_init(state_t * st, fstate_t * fs) {
    memset(st, NUL, sizeof(*st));
    st->ln_st   = fs->cur_line;
    st->ncln_st = fs->nc_line;
    st->fstate  = fs;
}

#define MAX_SCORE 999999

extern score_t penalty;
extern score_t subexp_penalty;
extern score_t scaling;

#define NESTING_PENALTY(_s)   ((_s) * penalty)

extern token_t
next_token(fstate_t * fs);

extern void
unget_token(fstate_t * fs);

extern bool
find_proc_start(fstate_t * fs);

extern void
do_column_totals(void);

extern void
decomment_file(char const * fname, FILE * fp);

extern void
initialize(int argc, char ** argv);

extern void
do_summary(complexity_exit_code_t);

extern void
score_proc(state_t * score);

extern void
die(complexity_exit_code_t, char const * fmt, ...);

#endif /* COMPLEXITY_H_GUARD */
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of complexity.h */
