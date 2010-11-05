/* pk-log.h
 *
 * Copyright (C) 2010 Christian Hergert <chris@dronelabs.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PK_LOG_H__
#define __PK_LOG_H__

#include <glib.h>
#include <time.h>

#ifndef G_LOG_LEVEL_TRACE
#define G_LOG_LEVEL_TRACE (1 << G_LOG_LEVEL_USER_SHIFT)
#endif

#define ERROR(_d, _f, ...)    g_log(#_d, G_LOG_LEVEL_ERROR, _f, ##__VA_ARGS__)
#define WARNING(_d, _f, ...)  g_log(#_d, G_LOG_LEVEL_WARNING, _f, ##__VA_ARGS__)
#define DEBUG(_d, _f, ...)    g_log(#_d, G_LOG_LEVEL_DEBUG, _f, ##__VA_ARGS__)
#define INFO(_d, _f, ...)     g_log(#_d, G_LOG_LEVEL_INFO, _f, ##__VA_ARGS__)
#define CRITICAL(_d, _f, ...) g_log(#_d, G_LOG_LEVEL_CRITICAL, _f, ##__VA_ARGS__)

#ifdef PERFKIT_TRACE
#define TRACE(_d, _f, ...) g_log(#_d, G_LOG_LEVEL_TRACE, "  MSG: " _f, ##__VA_ARGS__)
#define ENTRY                                                       \
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                          \
          "ENTRY: %s():%d", G_STRFUNC, __LINE__)
#define EXIT                                                        \
    G_STMT_START {                                                  \
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                      \
              " EXIT: %s():%d", G_STRFUNC, __LINE__);               \
        return;                                                     \
    } G_STMT_END
#define RETURN(_r)                                                  \
    G_STMT_START {                                                  \
    	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                      \
              " EXIT: %s():%d", G_STRFUNC, __LINE__);               \
        return _r;                                                  \
    } G_STMT_END
#define GOTO(_l)                                                    \
    G_STMT_START {                                                  \
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                      \
              " GOTO: %s:%d", #_l, __LINE__);                       \
        goto _l;                                                    \
    } G_STMT_END
#define CASE(_l)                                                    \
    case _l:                                                        \
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE, " CASE: %s:%d",      \
              #_l, __LINE__)
#define BREAK                                                       \
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                          \
          "BREAK: %s():%d", G_STRFUNC, __LINE__);                   \
    break
#define DUMP_MANIFEST(m)                                            \
    G_STMT_START {                                                  \
    	struct timespec _ts = { 0 };                                \
        gint _i, _n;                                                \
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                      \
              "  %30s = %d", "Resolution",                          \
              pk_manifest_get_resolution(m));                       \
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                      \
              "  %30s = %d", "Row Count",                           \
              pk_manifest_get_n_rows(m));                           \
        pk_manifest_get_timespec(m, &_ts);                          \
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                      \
              "  %30s = %lu.%09lu", "Timestamp",                    \
              _ts.tv_sec, _ts.tv_nsec);                             \
        _n = pk_manifest_get_n_rows(m);                             \
        for (_i = 1; _i <= _n; _i++) {                              \
            g_log(G_LOG_DOMAIN, G_LOG_LEVEL_TRACE,                  \
                  "  %30s = %s",                                    \
                  pk_manifest_get_row_name(m, _i),                  \
                  g_type_name(pk_manifest_get_row_type(m, _i)));    \
        }                                                           \
    } G_STMT_END
#else
#define TRACE(_d, _f, ...)
#define ENTRY
#define EXIT       return
#define RETURN(_r) return _r
#define GOTO(_l)   goto _l
#define CASE(_l)   case _l:
#define BREAK      break
#define DUMP_MANIFEST(m)
#endif

#define CASE_RETURN_STR(_l) case _l: return #_l

#endif /* __PK_LOG_H__ */
