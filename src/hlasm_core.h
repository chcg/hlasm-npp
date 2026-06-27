/*
 * hlasm_core.h -- Column-aware HLASM lexing logic, dependency-free.
 *
 * Pure parsing: takes a line, fills a style-byte array, returns the
 * continuation state. No Scintilla, no Notepad++, no Windows. Shared by
 * both the direct-styling plugin and the ILexer5 lexer so there is one
 * brain, lexed identically in both.
 *
 * 80-column card layout (0-indexed):
 *   Col 0-70    Content (label, operation, operand, remarks)
 *   Col 71      Continuation marker (non-blank = next line continues)
 *   Col 72-79   Sequence number
 */

#ifndef HLASM_CORE_H
#define HLASM_CORE_H

#include <cstring>
#include <cctype>

/* ---- Style IDs ---- */
enum {
    S_DEFAULT = 0, S_LABEL, S_OPERATION, S_OPERAND, S_COMMENT,
    S_STRING, S_NUMBER, S_CONTINUATION, S_SEQUENCE, S_REGISTER
};
#define HLASM_NUM_STYLES 10

#define MAX_LINE 256
#define COL72    71

/* ---- Helpers ---- */

static inline bool IsNameChar(char ch) {
    unsigned char c = (unsigned char)ch;
    return isalnum(c) || c == '@' || c == '#' || c == '$' || c == '_';
}

static inline int IsRegister(const char *buf, int pos, int limit) {
    if (pos >= limit) return 0;
    if (buf[pos] != 'R' && buf[pos] != 'r') return 0;
    int p = pos + 1;
    if (p >= limit || !isdigit((unsigned char)buf[p])) return 0;
    int d1 = buf[p] - '0';
    p++;
    if (p >= limit || !isdigit((unsigned char)buf[p])) {
        if (p < limit && IsNameChar(buf[p])) return 0;
        return 2;
    }
    int regnum = d1 * 10 + (buf[p] - '0');
    p++;
    if (regnum < 10 || regnum > 15) return 0;
    if (p < limit && IsNameChar(buf[p])) return 0;
    return 3;
}

static inline bool IsTypePrefix(char ch) {
    char u = (char)toupper((unsigned char)ch);
    return u == 'C' || u == 'X' || u == 'B' || u == 'F' || u == 'H' ||
           u == 'P' || u == 'Z' || u == 'A' || u == 'E' || u == 'D' ||
           u == 'Y' || u == 'V' || u == 'S' || u == 'G' || u == 'J';
}

/* ---- Operand sub-parser (fills sty[] from col to endCol) ---- */

static inline bool LexOperands(const char *buf, char *sty, int col, int endCol, bool inStr) {
    int parenDepth = 0;

    while (col < endCol) {
        if (inStr) {
            int start = col;
            bool closed = false;
            while (col < endCol) {
                if (buf[col] == '\'') {
                    col++;
                    if (col < endCol && buf[col] == '\'') col++;
                    else { closed = true; break; }
                } else col++;
            }
            memset(sty + start, S_STRING, col - start);
            if (!closed) return true;
            inStr = false;
            continue;
        }

        char ch = buf[col];

        if (ch == ' ' && parenDepth > 0) {
            sty[col] = S_OPERAND;
            col++;
            continue;
        }

        if (ch == ' ') {
            while (col < endCol && buf[col] == ' ') col++;
            if (col < endCol)
                memset(sty + col, S_COMMENT, endCol - col);
            col = endCol;
            return false;
        }

        if (ch == '(') parenDepth++;
        else if (ch == ')' && parenDepth > 0) parenDepth--;

        if (isalpha((unsigned char)ch) && IsTypePrefix(ch)) {
            int probe = col + 1;
            if (probe < endCol && toupper((unsigned char)buf[probe]) == 'L') {
                int saved = probe;
                probe++;
                while (probe < endCol && isdigit((unsigned char)buf[probe])) probe++;
                if (probe == saved + 1) probe = saved;
            }
            if (probe < endCol && buf[probe] == '\'') {
                if (col == 0 || !IsNameChar(buf[col - 1])) {
                    memset(sty + col, S_STRING, probe - col + 1);
                    col = probe + 1;
                    inStr = true;
                    continue;
                }
            }
        }

        if (isalpha((unsigned char)ch) || ch == '&' || ch == '.') {
            int rlen = IsRegister(buf, col, endCol);
            if (rlen > 0) {
                memset(sty + col, S_REGISTER, rlen);
                col += rlen;
                continue;
            }
            int sym = col;
            while (col < endCol && (IsNameChar(buf[col]) || buf[col] == '&' || buf[col] == '.')) col++;
            memset(sty + sym, S_OPERAND, col - sym);
            continue;
        }

        if (ch == '\'') {
            sty[col] = S_STRING;
            col++;
            inStr = true;
            continue;
        }

        if (isdigit((unsigned char)ch)) {
            int start = col;
            while (col < endCol && isdigit((unsigned char)buf[col])) col++;
            memset(sty + start, S_NUMBER, col - start);
            continue;
        }

        sty[col] = S_OPERAND;
        col++;
    }
    return inStr;
}

/* ---- Line lexer (fills sty[0..n-1], returns continuation state 0/1/2) ---- */

static inline int LexLine(const char *buf, char *sty, int n, int prevState) {
    if (n <= 0) return 0;

    int lim = (n > COL72) ? COL72 : n;
    int col = 0;
    bool inStr = false;

    /* Default everything first */
    memset(sty, S_DEFAULT, n);

    /* Full-line comment */
    if (buf[0] == '*' || (n > 1 && buf[0] == '.' && buf[1] == '*')) {
        memset(sty, S_COMMENT, lim);
        goto tail;
    }

    /* Continuation from previous line */
    if (prevState >= 1) {
        while (col < lim && buf[col] == ' ') col++;
        inStr = (prevState == 2);
        goto operands;
    }

    /* Label field */
    if (buf[0] != ' ') {
        while (col < lim && buf[col] != ' ') col++;
        memset(sty, S_LABEL, col);
    }

    /* Skip blanks to operation */
    while (col < lim && buf[col] == ' ') col++;

    /* Operation field */
    if (col < lim && buf[col] != ' ') {
        int op = col;
        while (col < lim && buf[col] != ' ') col++;
        memset(sty + op, S_OPERATION, col - op);
    }

    /* Skip blanks to operand */
    while (col < lim && buf[col] == ' ') col++;

operands:
    if (col < lim)
        inStr = LexOperands(buf, sty, col, lim, inStr);

tail:
    if (n > COL72) {
        bool cont = (buf[COL72] != ' ');
        sty[COL72] = cont ? S_CONTINUATION : S_DEFAULT;
        if (n > COL72 + 1) {
            int seq = n - (COL72 + 1);
            if (seq > 8) seq = 8;
            memset(sty + COL72 + 1, S_SEQUENCE, seq);
        }
        if (cont) return inStr ? 2 : 1;
    }
    return 0;
}

#endif /* HLASM_CORE_H */
