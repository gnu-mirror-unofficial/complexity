/*
 *  7 bits for 7 character classifications
 *  generated by char-mapper on 10/13/20 at 12:18:56
 *
 *  This file contains the caracter classifications used by the
 *  code complexity measurement tool.
 */
#ifndef CHAR_TYPES_H_GUARD
#define CHAR_TYPES_H_GUARD 1

#ifdef HAVE_CONFIG_H
# if defined(HAVE_INTTYPES_H)
#   include <inttypes.h>

# elif defined(HAVE_STDINT_H)
#   include <stdint.h>

#   elif !defined(HAVE_UINT8_T)
        typedef unsigned char   uint8_t;
# endif /* HAVE_*INT*_H header */

#else /* not HAVE_CONFIG_H -- */
# include <inttypes.h>
#endif /* HAVE_CONFIG_H */

#if 0 /* mapping specification source (from char-types.map) */
// 
// #  This file is part of Complexity.
// #  Complexity Copyright (c) 2011-2016 by Bruce Korb - all rights reserved
// #
// #  Complexity is free software: you can redistribute it and/or modify it
// #  under the terms of the GNU General Public License as published by the
// #  Free Software Foundation, either version 3 of the License, or
// #  (at your option) any later version.
// #
// #  Complexity is distributed in the hope that it will be useful, but
// #  WITHOUT ANY WARRANTY; without even the implied warranty of
// #  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// #  See the GNU General Public License for more details.
// #
// #  You should have received a copy of the GNU General Public License along
// #  with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// %guard      define-cplx-char-type
// %file       char-types.h
// %table      cplx-char-type
// 
// %comment -- see above
// %
// 
// alpha       "a-zA-Z"
// digit       "0-9"
// name-start  "_$" +alpha
// name        "$" +name-start +digit
// space       " \t\b\r\v\f\n"
// end-of-line "\n\r\0"
// star-or-nl  "*" +end-of-line
//
#endif /* 0 -- mapping spec. source */


typedef uint8_t cplx_char_type_mask_t;

#define  IS_ALPHA_CHAR( _c)         is_cplx_char_type_char((char)(_c), 0x01)
#define SPN_ALPHA_CHARS(_s)        spn_cplx_char_type_chars(_s, 0x01)
#define BRK_ALPHA_CHARS(_s)        brk_cplx_char_type_chars(_s, 0x01)
#define  IS_DIGIT_CHAR( _c)         is_cplx_char_type_char((char)(_c), 0x02)
#define SPN_DIGIT_CHARS(_s)        spn_cplx_char_type_chars(_s, 0x02)
#define BRK_DIGIT_CHARS(_s)        brk_cplx_char_type_chars(_s, 0x02)
#define  IS_NAME_START_CHAR( _c)    is_cplx_char_type_char((char)(_c), 0x05)
#define SPN_NAME_START_CHARS(_s)   spn_cplx_char_type_chars(_s, 0x05)
#define BRK_NAME_START_CHARS(_s)   brk_cplx_char_type_chars(_s, 0x05)
#define  IS_NAME_CHAR( _c)          is_cplx_char_type_char((char)(_c), 0x0F)
#define SPN_NAME_CHARS(_s)         spn_cplx_char_type_chars(_s, 0x0F)
#define BRK_NAME_CHARS(_s)         brk_cplx_char_type_chars(_s, 0x0F)
#define  IS_SPACE_CHAR( _c)         is_cplx_char_type_char((char)(_c), 0x10)
#define SPN_SPACE_CHARS(_s)        spn_cplx_char_type_chars(_s, 0x10)
#define BRK_SPACE_CHARS(_s)        brk_cplx_char_type_chars(_s, 0x10)
#define  IS_END_OF_LINE_CHAR( _c)   is_cplx_char_type_char((char)(_c), 0x20)
#define SPN_END_OF_LINE_CHARS(_s)  spn_cplx_char_type_chars(_s, 0x20)
#define BRK_END_OF_LINE_CHARS(_s)  brk_cplx_char_type_chars(_s, 0x20)
#define  IS_STAR_OR_NL_CHAR( _c)    is_cplx_char_type_char((char)(_c), 0x60)
#define SPN_STAR_OR_NL_CHARS(_s)   spn_cplx_char_type_chars(_s, 0x60)
#define BRK_STAR_OR_NL_CHARS(_s)   brk_cplx_char_type_chars(_s, 0x60)

#ifdef DEFINE_CPLX_CHAR_TYPE
cplx_char_type_mask_t const cplx_char_type[128] = {
  /*NUL*/ 0x20, /*x01*/ 0x00, /*x02*/ 0x00, /*x03*/ 0x00,
  /*x04*/ 0x00, /*x05*/ 0x00, /*x06*/ 0x00, /*BEL*/ 0x00,
  /* BS*/ 0x10, /* HT*/ 0x10, /* NL*/ 0x30, /* VT*/ 0x10,
  /* FF*/ 0x10, /* CR*/ 0x30, /*x0E*/ 0x00, /*x0F*/ 0x00,
  /*x10*/ 0x00, /*x11*/ 0x00, /*x12*/ 0x00, /*x13*/ 0x00,
  /*x14*/ 0x00, /*x15*/ 0x00, /*x16*/ 0x00, /*x17*/ 0x00,
  /*x18*/ 0x00, /*x19*/ 0x00, /*x1A*/ 0x00, /*ESC*/ 0x00,
  /*x1C*/ 0x00, /*x1D*/ 0x00, /*x1E*/ 0x00, /*x1F*/ 0x00,
  /*   */ 0x10, /* ! */ 0x00, /* " */ 0x00, /* # */ 0x00,
  /* $ */ 0x0C, /* % */ 0x00, /* & */ 0x00, /* ' */ 0x00,
  /* ( */ 0x00, /* ) */ 0x00, /* * */ 0x40, /* + */ 0x00,
  /* , */ 0x00, /* - */ 0x00, /* . */ 0x00, /* / */ 0x00,
  /* 0 */ 0x02, /* 1 */ 0x02, /* 2 */ 0x02, /* 3 */ 0x02,
  /* 4 */ 0x02, /* 5 */ 0x02, /* 6 */ 0x02, /* 7 */ 0x02,
  /* 8 */ 0x02, /* 9 */ 0x02, /* : */ 0x00, /* ; */ 0x00,
  /* < */ 0x00, /* = */ 0x00, /* > */ 0x00, /* ? */ 0x00,
  /* @ */ 0x00, /* A */ 0x01, /* B */ 0x01, /* C */ 0x01,
  /* D */ 0x01, /* E */ 0x01, /* F */ 0x01, /* G */ 0x01,
  /* H */ 0x01, /* I */ 0x01, /* J */ 0x01, /* K */ 0x01,
  /* L */ 0x01, /* M */ 0x01, /* N */ 0x01, /* O */ 0x01,
  /* P */ 0x01, /* Q */ 0x01, /* R */ 0x01, /* S */ 0x01,
  /* T */ 0x01, /* U */ 0x01, /* V */ 0x01, /* W */ 0x01,
  /* X */ 0x01, /* Y */ 0x01, /* Z */ 0x01, /* [ */ 0x00,
  /* \ */ 0x00, /* ] */ 0x00, /* ^ */ 0x00, /* _ */ 0x04,
  /* ` */ 0x00, /* a */ 0x01, /* b */ 0x01, /* c */ 0x01,
  /* d */ 0x01, /* e */ 0x01, /* f */ 0x01, /* g */ 0x01,
  /* h */ 0x01, /* i */ 0x01, /* j */ 0x01, /* k */ 0x01,
  /* l */ 0x01, /* m */ 0x01, /* n */ 0x01, /* o */ 0x01,
  /* p */ 0x01, /* q */ 0x01, /* r */ 0x01, /* s */ 0x01,
  /* t */ 0x01, /* u */ 0x01, /* v */ 0x01, /* w */ 0x01,
  /* x */ 0x01, /* y */ 0x01, /* z */ 0x01, /* { */ 0x00,
  /* | */ 0x00, /* } */ 0x00, /* ~ */ 0x00, /*x7F*/ 0x00
};
#endif /* DEFINE_CPLX_CHAR_TYPE */
extern cplx_char_type_mask_t const cplx_char_type[128];
static inline int
is_cplx_char_type_char(char ch, cplx_char_type_mask_t mask)
{
    unsigned int ix = (unsigned char)ch;
    return ((ix < 128) && ((cplx_char_type[ix] & mask) != 0));
}

static inline char *
spn_cplx_char_type_chars(char const * p, cplx_char_type_mask_t mask)
{
    while ((*p != '\0') && is_cplx_char_type_char(*p, mask))  p++;
    return (char *)(uintptr_t)p;
}

static inline char *
brk_cplx_char_type_chars(char const * p, cplx_char_type_mask_t mask)
{
    while ((*p != '\0') && (! is_cplx_char_type_char(*p, mask)))  p++;
    return (char *)(uintptr_t)p;
}
#endif /* CHAR_TYPES_H_GUARD */
/* sum: char-types.map = 55763.2 */
