// This file is distributed under MIT-LICENSE. See COPYING for details.

#include "json.h"
#include "stringarray.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>


char const * * bear_json_escape_strings(char const * * raw)
{
    char const * * it = raw;
    for (; (raw) && (*it); ++it)
    {
        char const * const new = bear_json_escape_string(*it, /*handle_space*/1);
        if (new)
        {
            char const * const tmp = *it;
            *it = new;
            free((void *)tmp);
        }
    }
    return raw;
}

static size_t count(char const * const begin,
                    char const * const end,
                    int(*fp)(int));

static int needs_escape(int);
static int needs_str_escape(int);

char const * bear_json_escape_string(char const * raw, int handle_space)
{
    size_t const length = (raw) ? strlen(raw) : 0;
    size_t const spaces = length && handle_space ? count(raw, raw + length, isspace) : 0;
    size_t const json = count(raw, raw + length, needs_escape);

    if ((0 == spaces) && (0 == json))
    {
        return 0;
    }

    char * const result = malloc(length + ((0 != spaces) * 4) + json + 1);
    if (0 == result)
    {
        perror("xcalbuild: malloc");
        exit(EXIT_FAILURE);
    }
    char * it = result;
    if (spaces)
    {
        *it++ = '\\';
        *it++ = '\"';
    }
    for (; (raw) && (*raw); ++raw)
    {
        if (needs_str_escape(*raw))
        {
            *it++ = '\\';
        }
        *it++ = isspace(*raw) ? ' ' : *raw;
    }
    if (spaces)
    {
        *it++ = '\\';
        *it++ = '\"';
    }
    *it = '\0';
    return result;
}

static size_t count(char const * const begin,
                    char const * const end,
                    int (*fp)(int))
{
    size_t result = 0;
    char const * it = begin;
    for (; it != end; ++it)
    {
        if (fp(*it))
        {
            ++result;
        }
    }
    return result;
}

static int needs_escape(int c)
{
    switch (c)
    {
    case '\\':
    case '\"':
    // Also need to replace \r\n inside a string.
    case '\n':
    case '\r':
        return 1;
    }
    return 0;
}

static int needs_str_escape(int c)
{
    switch (c)
    {
    case '\\':
    case '\"':
    // \r \n are handled by isspace().
        return 1;
    }
    return 0;
}
