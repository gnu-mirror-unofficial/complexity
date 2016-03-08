
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

#define _GNU_SOURCE 1

#include "opts.h"
#include <limits.h>
#include <math.h>
#include <regex.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef UNIFDEF_EXE
#define UNIFDEF_EXE "unifdef"
#endif

#define RANGE_LIMIT 2000

static score_t score_ttl      = 0;
static int     ttl_line_ct    = 0;
static int     unscore_ct     = 0;
static int     high_score     = 0;
       score_t subexp_penalty = 0;
       score_t threshold      = 0;
       score_t scaling;

static char const head_fmt[] =     "Complexity Scores\n"
    "Score | ln-ct | nc-lns| file-name(line): proc-name\n";
static char const line_fmt[] =     "%5d  %6d  %6d   %s(%d): %s\n";
static char const lnct_fmt[] =     "total nc-lns %8d\n";
static char const bad_line_fmt[] = "***** %6d %6d %s(%d): %s\n";
static char const nomem_fmt[] =    "could not allocate %d bytes\n";

static int     score_alloc_ct = 0;
static state_t ** scores      = NULL;

static char const * unifcmd = UNIFDEF_EXE;
static char high_buf[1024];

void
initialize(int argc, char ** argv)
{
    if (HAVE_OPT(INPUT) && (argc > 0)) {
        static char const oops[] =
            "source files were specified both on the command line "
            "and with the --input option.\n";
        fwrite(oops, sizeof(oops)-1, 1, stderr);
        USAGE(EXIT_FAILURE);
    }

    if (! HAVE_OPT(SCORES)) {
        if (! ENABLED_OPT(HISTOGRAM))
            SET_OPT_SCORES;

        else if (! HAVE_OPT(THRESHOLD))
            /*
             * No scores, doing a histogram and have not specified a threshold.
             */
            SET_OPT_THRESHOLD(0);
    }

    if (HAVE_OPT(NESTING_PENALTY)) {
        penalty = atof(OPT_ARG(NESTING_PENALTY));
        if (penalty < 1.0)
            penalty = DEFAULT_PENALTY;
    }

    if (HAVE_OPT(DEMI_NESTING_PENALTY))
        subexp_penalty = atof(OPT_ARG(DEMI_NESTING_PENALTY));

    if (subexp_penalty < 1.0)
        subexp_penalty = sqrt(penalty);

    threshold = (score_t)OPT_VALUE_THRESHOLD - 0.5;

    scores = malloc(1024 * sizeof(*scores));
    score_alloc_ct = 1024;
    scaling = (score_t)(HAVE_OPT(SCALE) ? OPT_VALUE_SCALE : DEFAULT_SCALE);
    scaling = 1.0 / scaling;
}

static inline int
hash_score(score_t score)
{
    int sc = score + 0.5;
    if (sc < 10)   return 0;
    if (sc < 100)  return sc / 10; // 1 -> 9
    if (sc < 1000) return 9 + (sc / 100); // 10 -> 18
    return 18 + (sc / 1000); // 19 -> ...
}

static inline bool
check_skip(int * scores, int ix, int lim, bool skip_now)
{
    if (skip_now)
        return skip_now; // have zero and already skipping
    /*
     *  First zero count in a sequence.
     */
    if ((ix + 1) < lim)
        return false; // no more lines, so do not ignore.
    if (scores[ix + 1] != 0)
        return false; // next not zero, so report zero
    return true; // next is zero too, so start zero sequence
}

static void
print_histogram(void)
{
    static char const hsthdr[] =
        "Complexity Histogram\n"
        "Score-Range  Lin-Ct\n";
    static char const stars[] =
        "************************************************************";
    static int  const starct = sizeof(stars) - 1;
    static char const fmtfmt[] = "%%5d-%%-5d %%7d %%%1$d.%1$ds\n";
    static char const deffmt[] = "%5d-%-5d %7d\n";

    int const score_ix_lim = hash_score(scores[score_ct-1]->score) + 1;

    int * lines_scoring = calloc(score_ix_lim, sizeof(int));
    int max_ct = 0;

    for (int ix = 0; ix < score_ct; ix++) {
        int score_ix = hash_score(scores[ix]->score);

        lines_scoring[score_ix] += scores[ix]->st_nc_line_ct;
        if (lines_scoring[score_ix] > max_ct)
            max_ct = lines_scoring[score_ix];
    }

    if (! HAVE_OPT(NO_HEADER)) {
        if (ENABLED_OPT(SCORES))
            putc(NL, stdout);
        fwrite(hsthdr, sizeof(hsthdr) - 1, 1, stdout);
    }

    int  min_score  = 0;
    int  score_inc  = 10;
    int  hi_score   = min_score + score_inc - 1;
    bool skipping   = true;
    bool first_line = true;

    for (int ix = 0;
         ix < score_ix_lim;
         ix++, min_score = hi_score + 1, hi_score += score_inc) {
        int  ct     = lines_scoring[ix];
        bool put_nl = false;

        switch (min_score) {
        case 90:  score_inc = 100;  put_nl = true; break;
        case 900: score_inc = 1000; put_nl = true; break;
        }

        if (ct == 0) {
            skipping = check_skip(lines_scoring, ix, score_ix_lim, skipping);
            if (skipping)
                continue;
        }

        /*
         * We are not skipping any more.  If we were, put out a marker.
         */
        if (skipping) {
            if (! first_line)
                fputs("**********\n", stdout);
            skipping = first_line = false;
        }

        int  width = ((starct * ct) + (max_ct/2)) / max_ct;
        if (width > 0) {
            char fmt[64];
            width = snprintf(fmt, sizeof(fmt), fmtfmt, width);
            printf(fmt, min_score, hi_score, ct, stars);
        } else
            printf(deffmt, min_score, hi_score, ct);

        if (put_nl)
            putc(NL, stdout);
    }

    free(lines_scoring);
}

static void
print_stats(void)
{
#define SUMMARY_TABLE                                                   \
    _St_("Scored procedure ct:  %7d\n", score_ct)                       \
    _St_("Non-comment line ct:  %7d\n", ttl_line_ct)                    \
    _St_("Average line score:   %7d\n",                                 \
         (int)(av_score + 0.5))                                         \
    _St_("25%%-ile score:        %7d (75%% in higher score procs)\n",   \
         pctile[0])                                                     \
    _St_("50%%-ile score:        %7d (half in higher score procs)\n",   \
         pctile[1])                                                     \
    _St_("75%%-ile score:        %7d (25%% in higher score procs)\n",   \
         pctile[2])                                                     \
    _St_("Highest score:        %7d",   high_score)                     \
    _St_(" (%s)\n",                     high_buf)

#define _St_(_s, _a)  _s
    static char const summary_fmt[] = "\n" SUMMARY_TABLE;
#undef  _St_

    score_t av_score     = score_ttl / ttl_line_ct;
    int     pctile[5]    = { 0, 0, 0, 0, 0 };
    int     pct_ix       = 0;
    int     counter      = 0;
    int     ix;
    int     pct_ct       = ttl_line_ct / 4;
    int     pct_thresh   = pct_ct;

    for (ix = 0; ix < score_ct; ix++) {
        counter += scores[ix]->st_nc_line_ct;

        if (counter >= pct_thresh) {
            pctile[pct_ix++] = (int)(scores[ix]->score + 0.5);
            pct_thresh      += pct_ct;
        }
    }

#define _St_(_s, _a)  , _a
    printf(summary_fmt SUMMARY_TABLE);
#undef  _St_

    if (unscore_ct > 0)
        printf("Unscored procedures:  %7d\n", unscore_ct);
#undef  SUMMARY_TABLE
}

static int
compare_score(void const * a, void const * b)
{
    state_t * A = *(void **)a;
    state_t * B = *(void **)b;
    int res = (int)(A->score - B->score);
    if (res != 0)
        return res;
    res = A->st_nc_line_ct - B->st_nc_line_ct;
    if (res != 0)
        return res;
    return A->st_line_ct - B->st_line_ct;
}

void
do_summary(complexity_exit_code_t exit_code)
{
    qsort(scores, score_ct, sizeof(state_t *), compare_score);
    if (ENABLED_OPT(SCORES)) {
        if (! HAVE_OPT(NO_HEADER))
            fwrite(head_fmt, sizeof(head_fmt) - 1, 1, stdout);

        for (int ix = 0; ix < score_ct; ix++) {
            int val = scores[ix]->score + 0.5;
            printf(line_fmt, val, scores[ix]->st_line_ct,
                   scores[ix]->st_nc_line_ct,
                   scores[ix]->st_end, scores[ix]->ln_st, scores[ix]->pname);
        }
    }
    if (HAVE_OPT(HISTOGRAM)) {
        print_histogram();
        print_stats();

    } else if (! HAVE_OPT(NO_HEADER))
        printf(lnct_fmt, ttl_line_ct);
}

static FILE *
popen_unifdef(char const * fname)
{
    static char const * cmd = NULL;
    static size_t       cmdlen = 0;

    if (cmd == NULL) {
        int ct = STACKCT_OPT(UNIFDEF);
        char const * const * ov = STACKLST_OPT(UNIFDEF);
        char * buf;
        size_t len;

        /*
         * Select the correct unifdef command and add one to the length:
         * for NUL termination.  Then add up the lengths of the arguments
         * and space separations we need.
         */
        if (HAVE_OPT(UNIF_EXE))
            unifcmd = OPT_ARG(UNIF_EXE);
        len = cmdlen = strlen(unifcmd) + 1;

        for (int i = 0; i < ct; i++)
            len += 1 + strlen(ov[i]);

        cmd = buf = malloc(len);
        if (buf == NULL)
            die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, len);

        /*
         * Now assemble the command
         */
        memcpy(buf, unifcmd, cmdlen);
        buf += cmdlen - 1;

        while (ct-- > 0) {
            char const * p = *(ov++);
            *(buf++) = ' ';
            len = strlen(p);
            memcpy(buf, p, len);
            buf += len;
        }
        *buf   = NUL;
        cmdlen = (buf - cmd) + 2;
    }

    {
        size_t bfsz = cmdlen + strlen(fname);
        char * bf   = malloc(bfsz);
        FILE * res;
        if (bf == NULL)
            die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, bfsz);
        size_t sz = snprintf(bf, bfsz, "%s %s", cmd, fname);
        CX_ASSERT(sz < bfsz);

        res  = popen(bf, "r");
        free(bf);
        return res;
    }
}

static bool
load_file(fstate_t * fs)
{
    unsigned long const pgsz = sysconf(_SC_PAGE_SIZE);
    off_t fsiz = pgsz * 4;
    off_t foff = 0;
    bool  is_guess = true;
    char * full_text;
    char * rdp;

    {
        struct stat sb;
        if (fstat(fileno(fs->fs_fp), &sb) >= 0) {
            if (S_ISREG(sb.st_mode)) {
                fsiz = sb.st_size + 1;
                is_guess = false;
            }
        }
    }

    rdp = full_text = malloc(fsiz);
    if (full_text == NULL)
        die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, fsiz);

    for (;;) {
        size_t ct   = fsiz - foff;
        size_t rdct = fread(rdp, 1, ct, fs->fs_fp);

        if ((rdct < ct) || (! is_guess)) {
            rdp[rdct] = '\0';
            break;
        }

        /*
         *  We've filled out buffer and we don't know final size.
         *
         *  4, 6, 9, 13, 19, 28, ... pages
         */
        fsiz += (fsiz / 2) & ~(pgsz - 1);
        full_text = realloc(full_text, fsiz);
        if (full_text == NULL)
            die(COMPLEXITY_EXIT_NOMEM,
                "reallocation of %d to %d bytes failed\n",
                (fsiz * 2) / 3, fsiz);

        foff += rdct;
        rdp  = full_text + foff;
    }

    fs->fs_text  = fs->fs_scan = full_text;
    fs->cur_line = 1;
    fs->nc_line  = 0;
    fs->fs_bol   = true;
    fs->last_tkn = TKN_EOF;
    if (HAVE_OPT(TRACE))
        fprintf(trace_fp, "\nLoading file %s\n", fs->fs_fname);
    return true;
}

static bool
add_score(state_t * pstate)
{
    if (threshold > pstate->score)
        return false;

    int val = (int)(pstate->score);

    if (val >= MAX_SCORE) {
        fprintf(stderr, "unscored: %s in %s on line %d\n",
                pstate->pname, pstate->st_fstate->fs_fname, pstate->proc_line);
        unscore_ct++;
        return false;
    }

    if (val > high_score) {
        snprintf(high_buf, sizeof(high_buf), "%s() in %s",
                 pstate->pname, pstate->st_fstate->fs_fname);
        high_score = val;
    }

    return true;
}

static void
re_die(int code, regex_t * re, char const * pat)
{
    char msg[64];
    (void)regerror(code, re, msg, sizeof(msg));
    die(COMPLEXITY_EXIT_ASSERT, "re error %s (%u) compiling ''%s''\n",
        msg, (unsigned)code, pat);
}

static regex_t *
re_compile(void)
{
    static char const pat[] = "[\r\n]\\}";

    regex_t * res = malloc(sizeof(*res));
    if (res == NULL)
        die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, (int)sizeof(*res));

    {
        int reres = regcomp(res, pat, REG_EXTENDED);
        if (reres != 0)
            re_die(reres, res, pat);
    }

    return res;
}

static bool
find_proc_end(state_t * ps, regex_t * re)
{
    regmatch_t match;
    char * scan = BRK_END_OF_LINE_CHARS(ps->st_fstate->fs_scan);

    /*
     * special case a one-line function (where opening and closing
     * braces are on the same line).
     */
    {
        char * close_brace = strchr(ps->st_fstate->fs_scan, '}');
        if (close_brace == NULL)
            return false;

        if (close_brace < scan) {
            ps->st_end = close_brace + 1; // same line
            return true;
        }
    }

    /*
     * run the regex looking for a CR or LF preceding a '}'
     */
    if (regexec(re, scan, 1, &match, 0) != 0)
        return false;
    ps->st_end = scan + match.rm_eo;
    return true;
}

static bool
do_proc(fstate_t * fs)
{
    bool res = true;
    static regex_t * re = NULL;
    state_t * pstate = malloc(sizeof(*pstate));
    if (pstate == NULL)
        die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, (int)sizeof(*pstate));

    if (re == NULL)
        re = re_compile();

    state_init(pstate, fs);

    if (! find_proc_end(pstate, re))
        goto all_done;

    if (HAVE_OPT(IGNORE)) {
        int ct = STACKCT_OPT(IGNORE);
        char const ** il = STACKLST_OPT(IGNORE);

        do  {
            if (strcmp(*(il++), pstate->pname) == 0)
                goto skip_proc;
        } while (--ct > 0);
    }

    pstate->proc_line = fs->cur_line;

    score_proc(pstate);
    if (! add_score(pstate))
        goto skip_proc;

    if (pstate->st_nc_line_ct == 0) {
        pstate->score = 0;
    } else {
        score_ttl   += (pstate->score * pstate->st_nc_line_ct);
        ttl_line_ct += pstate->st_nc_line_ct;
    }

    if (++score_ct >= score_alloc_ct) {
        score_alloc_ct += score_alloc_ct / 2;
        size_t sz = score_alloc_ct * sizeof(*scores);
        scores = realloc(scores, sz);
        if (scores == NULL)
            die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, sz);
    }

    pstate->st_end     = (char *)fs->fs_fname;
    scores[score_ct-1] = pstate;
    return res;

 skip_proc:

    while (fs->fs_scan < pstate->st_end) {
        if (fs->fs_scan[0] == NL)
            fs->cur_line++;
        fs->fs_scan++;
    }

 all_done:

    free(pstate);
    return res;
}

complexity_exit_code_t
complex_eval(char const * fname)
{
    complexity_exit_code_t res = COMPLEXITY_EXIT_SUCCESS;

    fstate_t fstate = {
        .fs_fp    = HAVE_OPT(UNIFDEF) ? popen_unifdef(fname) : fopen(fname, "r"),
        .fs_fname = fname
    };

    if (fstate.fs_fp == NULL)
        return COMPLEXITY_EXIT_BAD_FILE;

    if (! load_file(&fstate))
        return COMPLEXITY_EXIT_BAD_FILE;

    int proc_ct = strlen(fname) + 1;
    fname = strdup(fname);
    if (fname == NULL)
        die(COMPLEXITY_EXIT_NOMEM, nomem_fmt, proc_ct);
    proc_ct = 0;

    while (find_proc_start(&fstate))
        if (! do_proc(&fstate))
            break;

    fflush(stdout);
    free((void *)fstate.fs_text);

    HAVE_OPT(UNIFDEF) ? pclose(fstate.fs_fp) : fclose(fstate.fs_fp);

    if (high_score > OPT_VALUE_HORRID_THRESHOLD)
        res = COMPLEXITY_EXIT_HORRID_FUNCTION;

    return res;
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of complexity.c */
