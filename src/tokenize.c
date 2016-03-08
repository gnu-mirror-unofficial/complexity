
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

#define DEFINE_CPLX_CHAR_TYPE

#include "opts.h"

static bool
skip_comment(fstate_t * fs)
{
    char const * p = fs->fs_scan + 1; // skip the '*' from the "/*"

    for (;;) {
        p = BRK_STAR_OR_NL_CHARS(p);

        switch (*p) {
        case NUL:
            fs->fs_scan = p;
            return false;

        case NL:
            fs->cur_line++;
            /* FALLTHROUGH */

        case CR:
            p++;
            break;

        case '*':
            if (*++p == '/') {
                fs->fs_scan = p + 1;
                return true;
            }
        }
    }
}

static bool
skip_to_eol(fstate_t * fs)
{
    fs->fs_scan = BRK_END_OF_LINE_CHARS(fs->fs_scan);
    switch (fs->fs_scan[0]) {
    case CR:
        if (fs->fs_scan[1] == NL)
            fs->fs_scan++;
        /* FALLTHROUGH */
    case NL:
        return true;
    default:
        return false;
    }
}

static token_t
bang_check(fstate_t * fs)
{
    if (fs->fs_scan[0] != '=')
        return TKN_ARITH_OP; // logical not
    fs->fs_scan++;
    return TKN_REL_OP;       // not equal
}

static token_t
check_quote(fstate_t * fs, char q)
{
    token_t res = TKN_NAME;
    char const * s = fs->fs_scan;

    while (*s != q) {
        switch (*s) {
        case '\\':
            if (*++s != NUL)
                break;
            /* FALLTHROUGH */

        case NUL:
            res = TKN_EOF;
            goto leave;

        default: ;
        }
        s++;
    }

leave:
    fs->fs_scan = s + 1;
    return res; // string, actually
}

static token_t
dblquot_check(fstate_t * fs)
{
    return check_quote(fs, '"');
}

static token_t
sglquot_check(fstate_t * fs)
{
    return check_quote(fs, '\'');
}

/**
 * handle comments
 */
static inline bool
comment_check(fstate_t * fs, char const * s, token_t * res)
{
    fs->fs_scan = s;
    switch (*s) {
    case '*': // "C" comment
        if (! skip_comment(fs)) {
            *res = TKN_EOF;
            return true;
        }
        return false;

    case '/': // "C++" comment to end of line
        if (! skip_to_eol(fs))
            *res = TKN_EOF;
        return true;

    case CR:
        if (s[1] == NL)
            fs->fs_scan++;
        /* FALLTHROUGH */
    case NL:
        return true;

    case NUL: // Wooops
        *res = TKN_EOF;
        return true;

    default:
        return false;
    }
}

/**
 * handle a preprocessing directive.  If the line ends with a backslash,
 * then the next line is considered part of that directive.
 */
static token_t
hash_check(fstate_t * fs)
{
    if (! fs->fs_bol)
        return TKN_ARITH_OP;

    token_t res = TKN_EMPTY;
    char const * s = fs->fs_scan;
    char ch;

    while (! IS_END_OF_LINE_CHAR(*s)) {
        switch (*s) {
        case '\\':
            ch = *++s;
            switch (ch) {
            case NUL:
                goto eof_token;
            case NL:
                fs->cur_line++;
            }
            break; // ignore the character whatever it is.

        case NUL:
        eof_token:
            res = TKN_EOF;
            goto leave;

        case '/':
            if (! comment_check(fs, ++s, &res))
                goto leave;
            s = fs->fs_scan; // now at next char to examine
            continue;

        default:
            break;
        }
        s++;
    }

    if ((s[0] == CR) && (s[1] == NL))
        s++;

 leave:
    fs->fs_scan = s;
    fs->fs_bol  = true;
    return res;
}

static token_t
assign_check(fstate_t * fs)
{
    if (fs->fs_scan[0] != '=')
        return TKN_ARITH_OP;
    fs->fs_scan++;
    return TKN_ASSIGN;
}

static token_t
amph_check(fstate_t * fs)
{
    if (fs->fs_scan[0] != '&')
        return assign_check(fs);
    fs->fs_scan++;
    return TKN_LOGIC_AND;
}

static token_t
dot_check(fstate_t * fs)
{
    token_t res = TKN_ARITH_OP;
    if ((fs->fs_scan[0] == '.') && (fs->fs_scan[1] == '.')) {
        fs->fs_scan += 2;
        res = TKN_ELLIPSIS;
    }
    return res;
}

static token_t
plus_check(fstate_t * fs)
{
    token_t res = TKN_ARITH_OP;

    switch (fs->fs_scan[0]) {
    case '+': res = TKN_ARITH_OP; fs->fs_scan++; break;
    case '=': res = TKN_ASSIGN;   fs->fs_scan++; break;
    case NUL: res = TKN_EOF; break;
    }

    return res;
}

static token_t
hyphen_check(fstate_t * fs)
{
    token_t res = TKN_ARITH_OP;

    switch (fs->fs_scan[0]) {
    case '>':
    case '-': res = TKN_ARITH_OP; fs->fs_scan++; break;
    case '=': res = TKN_ASSIGN;   fs->fs_scan++; break;
    case NUL: res = TKN_EOF; break;
    }

    return res;
}

static token_t
slash_check(fstate_t * fs)
{
    token_t res = TKN_ARITH_OP;

    switch (fs->fs_scan[0]) {
    case '/': skip_to_eol(fs); res = TKN_EMPTY;  break;
    case '=': fs->fs_scan++;      res = TKN_ASSIGN; break;
    case '*':
        if (! skip_comment(fs))
            res = TKN_EOF;
        else
            res = TKN_EMPTY;
        break;
    }

    return res;
}

static token_t
less_check(fstate_t * fs)
{
    token_t res = TKN_REL_OP;

    switch (fs->fs_scan[0]) {
    case '<': fs->fs_scan++;
        if (fs->fs_scan[0] == '=') {
            fs->fs_scan++;
            res = TKN_ASSIGN;
        }
        else
            res = TKN_ARITH_OP;
        break;

    case '=': fs->fs_scan++; break;
    }

    return res;
}

static token_t
equal_check(fstate_t * fs)
{
    token_t res = TKN_ASSIGN;
    if (fs->fs_scan[0] == '=') {
        fs->fs_scan++;
        res = TKN_REL_OP;
    }
    return res;
}

static token_t
greater_check(fstate_t * fs)
{
    token_t res = TKN_REL_OP;

    switch (fs->fs_scan[0]) {
    case '>': fs->fs_scan++;
        if (fs->fs_scan[0] == '=') {
            fs->fs_scan++;
            res = TKN_ASSIGN;
        }
        else
            res = TKN_ARITH_OP;
        break;

    case '=': fs->fs_scan++; break;
    }

    return res;
}

static token_t
carat_check(fstate_t * fs)
{
    token_t res = TKN_ARITH_OP;

    switch (fs->fs_scan[0]) {
    case '=': fs->fs_scan++;      res = TKN_ASSIGN; break;
    }

    return res;
}

static token_t
vertbar_check(fstate_t * fs)
{
    if (fs->fs_scan[0] != '|')
        return assign_check(fs);
    fs->fs_scan++;
    return TKN_LOGIC_OR;
}

static token_t
unknown_check(fstate_t * fs)
{
    unsigned char ch = fs->fs_scan[-1];

    fprintf(stderr, "invalid character in %s on line %d: 0x%02X (%c)\n",
            fs->fs_fname, fs->cur_line, ch,
            (isprint(ch) ? ch : '?'));

    return TKN_EOF;
}

static token_t
keyword_check(fstate_t * fs)
{
    static struct {
        char const *    name;
        token_t         tval;
        int             nlen;
    } const kw_table[] = {
#define _Ktbl_(_n, _e) { #_n, _e, sizeof(#_n) - 1 },
        RES_WORD_TABLE
#undef  _Ktbl_
    };
    static int const kw_size = sizeof(kw_table) / sizeof(kw_table[0]);

    int lo = 0;
    int hi = kw_size - 1;
    fs->fs_scan--;

    do  {
        int ix  = (lo + hi) / 2;
        int cmp = strncmp(kw_table[ix].name, fs->fs_scan, kw_table[ix].nlen);

        if (cmp == 0) {
            if (! IS_NAME_CHAR(fs->fs_scan[kw_table[ix].nlen])) {
                fs->fs_scan += kw_table[ix].nlen;
                return kw_table[ix].tval;
            }
            break;
        }

        if (cmp < 0)
            lo = ix + 1;
        else
            hi = ix - 1;
    } while (lo <= hi);

    for (;;) {
        fs->fs_scan = SPN_NAME_CHARS(fs->fs_scan + 1);
        if ((fs->fs_scan[0] != ':') || (fs->fs_scan[1] != ':'))
            return TKN_NAME;
        fs->fs_scan += 2;
    }
}

void
unget_token(fstate_t * fs)
{
    fs->fs_scan     = fs->tkn_text;
    fs->cur_line = fs->tkn_line;
}

token_t
extern_c_check(fstate_t * fs)
{
    int nl_ct = 0;
    char const * s = fs->fs_scan;
    while (IS_SPACE_CHAR(*s)) {
        if (*s == NL)
            nl_ct++;
        s++;
    }

    if (strncmp(s, "\"C\"", 3) != 0)
        return TKN_NAME;
    s += 3;
    while (IS_SPACE_CHAR(*s)) {
        if (*s == NL)
            nl_ct++;
        s++;
    }
    if (*s != '{') return TKN_NAME;
    fs->fs_scan      = s + 1;
    fs->cur_line += nl_ct;
    return TKN_EMPTY;
}

static inline bool
next_nonblank(fstate_t * fs)
{
    for (;;) {
        switch (*(fs->fs_scan)) {
        case NL:
            fs->cur_line++;
            fs->fs_bol = true;
            /* FALLTHROUGH */

        case ' ': case '\t': case '\f': case '\v': case CR:
            fs->fs_scan++;
            break;

        case NUL:
            return false;

        default:
            fs->tkn_text = fs->fs_scan;
            fs->tkn_line = fs->cur_line;
            return true;
        }
    }
}

token_t
next_token(fstate_t * fs)
{
    token_t res = TKN_EOF;

    do  {
        if (! next_nonblank(fs))
            return TKN_EOF;

        switch (*(fs->fs_scan++)) {
        case NUL:
            return TKN_EOF;

        case 'A' ... 'Z':
        case '_': case '$':
            fs->fs_scan = SPN_NAME_CHARS(fs->fs_scan + 1);
            res = TKN_NAME;
            break;

        case 'a' ... 'z':
            res = keyword_check(fs);
            if (res == TKN_KW_EXTERN)
                res = extern_c_check(fs);
            break;

        case '0' ... '9':
            fs->fs_scan = SPN_NAME_CHARS(fs->fs_scan);
            res = TKN_NUMBER;
            break;

        case '!':  res = bang_check(   fs); break;
        case '"':  res = dblquot_check(fs); break;
        case '#':  res = hash_check(   fs); break;
        case '%':  res = assign_check( fs); break;
        case '&':  res = amph_check(   fs); break;
        case '*':  res = assign_check( fs); break;
        case '+':  res = plus_check(   fs); break;
        case '-':  res = hyphen_check( fs); break;
        case '.':  res = dot_check(    fs); break;
        case '/':  res = slash_check(  fs); break;
        case '<':  res = less_check(   fs); break;
        case '=':  res = equal_check(  fs); break;
        case '>':  res = greater_check(fs); break;
        case '\'': res = sglquot_check(fs); break;
        case '^':  res = carat_check(  fs); break;
        case '|':  res = vertbar_check(fs); break;

        case '(':  res = TKN_LIT_OPNPAREN;  break;
        case ')':  res = TKN_LIT_CLSPAREN;  break;
        case ',':  res = TKN_LIT_COMMA;     break;
        case ';':  res = TKN_LIT_SEMI;      break;
        case '?':  res = TKN_LIT_QUESTION;  break;
        case '[':  res = TKN_LIT_OPNBRACK;  break;
        case ']':  res = TKN_LIT_CLSBRACK;  break;
        case '{':  res = TKN_LIT_OBRACE;    break;
        case '}':  res = TKN_LIT_CBRACE;    break;

        case '\\': res = TKN_EMPTY;         break;
        case '~':  res = TKN_ARITH_OP;      break;

        case ':':
            if (fs->fs_scan[0] == ':') {
                /*
                 * double colon name space operator -- basically
                 * part of the name.
                 */
                fs->fs_scan++;
                (void)keyword_check(fs);
                res = TKN_NAME;
            } else {
                res = TKN_LIT_COLON;
            }
            break;

        case '@':
        case '`':
        default:   res = unknown_check(fs); break;
        }
    } while (res == TKN_EMPTY);

    fs->tkn_len = fs->fs_scan - fs->tkn_text;
    if (fs->fs_bol) {
        fs->fs_bol = 0;
        fs->nc_line++;
    }

    return fs->last_tkn = res;
}

static token_t
skip_params(fstate_t * fs)
{
    int depth = 1;
    for (;;) {
        token_t tk = next_token(fs);
        switch (tk) {
        case TKN_LIT_OPNPAREN:
            depth++;
            break;

        case TKN_LIT_CLSPAREN:
            if (--depth > 0)
                continue;
            /* FALLTHROUGH */
        case TKN_EOF:
            return tk;

        default:
            continue;
        }
    }
}

/*
 * quit when you find either a semi-colon or a closing brace in col 1.
 */
static token_t
skip_to_semi(fstate_t * fs)
{
    int depth = (fs->last_tkn == TKN_LIT_OBRACE) ? 1 : 0;

    for (;;) {
        token_t tkn = next_token(fs);
        switch (tkn) {
        case TKN_LIT_OBRACE:
            depth++;
            break;

        case TKN_LIT_CBRACE:
            if ((--depth == 0) && (fs->tkn_text[-1] == '\n'))
                return tkn;
            break;

        case TKN_LIT_SEMI:
            if (depth)
                continue;
            /* FALLTHROUGH */
        case TKN_EOF:
            return tkn;

        default:
            continue;
        }
    }
}

bool
find_proc_start(fstate_t * fs)
{
    bool res = false;

    for (;;) {
        token_t tkn = next_token(fs);
        char const * proc_name;
        size_t       pname_len;

        switch (tkn) {
        case TKN_NAME:     break;
        case TKN_EOF:      return res;
        case TKN_LIT_SEMI: continue;
        default:
            tkn = skip_to_semi(fs);
            if (tkn == TKN_EOF)
                return res;
            continue;
        }

        do  {
            proc_name = fs->tkn_text;
            pname_len = fs->tkn_len;
            do
                tkn = next_token(fs);
            while (tkn == TKN_ARITH_OP);
        } while (tkn == TKN_NAME);

        switch (tkn) {
        case TKN_LIT_OPNPAREN: break;
        case TKN_EOF:          return res;
        case TKN_LIT_SEMI:     continue;
        default:
            tkn = skip_to_semi(fs);
            if (tkn == TKN_EOF)
                return res;
            continue;
        }

        tkn = skip_params(fs);

        switch (tkn) {
        case TKN_LIT_CLSPAREN: break;
        case TKN_EOF:          return res;
        case TKN_LIT_SEMI:     continue;
        default:
            tkn = skip_to_semi(fs);
            if (tkn == TKN_EOF)
                return res;
            continue;
        }

        tkn = next_token(fs);
        if (tkn == TKN_LIT_OBRACE) {
            fs->tkn_text = proc_name;
            fs->tkn_len  = pname_len;
            return true;
        }
    }
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of tokenize.c */
