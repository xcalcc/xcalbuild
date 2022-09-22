// This file is distributed under MIT-LICENSE. See COPYING for details.

#include "output.h"
#include "stringarray.h"
#include "protocol.h"
#include "json.h"

#include <unistd.h>
#include <libgen.h> // must be before string.h so we get POSIX basename
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>


static size_t count = 0;

int bear_open_json_output(char const * file)
{
    int fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    if (-1 == fd)
    {
        perror("xcalbuild: open");
        exit(EXIT_FAILURE);
    }
    dprintf(fd, "[\n");
    count = 0;
    return fd;
}

void bear_close_json_output(int fd)
{
    dprintf(fd, "]\n");
    close(fd);
}

static int is_known_compiler(char const * cmd);

void bear_append_json_output(int fd, struct bear_message const * e, int debug)
{
    int known = (e->cmd) && (e->cmd[0]) && is_known_compiler(e->cmd[0]);
    char const * const cmd = bear_strings_fold(bear_json_escape_strings(e->cmd), ',');
    if (debug)
    {
        if (count++)
        {
            dprintf(fd, ",\n");
        }
        char const * resp = bear_json_escape_string(e->resp_file, /*handle_space*/0);
        dprintf(fd,
                "{\n"
                "  \"pid\": \"%d\",\n"
                "  \"ppid\": \"%d\",\n"
                "  \"function\": \"%s\",\n"
                "  \"directory\": \"%s\",\n"
                "  \"arguments\": [%s],\n"
                "  \"respfile\": \"%s\",\n"
                "  \"isKnown\": %d\n"
                "}\n",
                e->pid, e->ppid, e->fun, e->cwd, cmd, resp ? resp : e->resp_file, known);
        free((void*)resp);
    }
    else if (known)
    {
        if (count++)
        {
            dprintf(fd, ",\n");
        }
        dprintf(fd,
            "{\n"
            "  \"directory\": \"%s\",\n"
            "  \"arguments\": [%s]",
            e->cwd, cmd);
        if (strlen(e->resp_file))
        {
            char const * resp = bear_json_escape_string(e->resp_file, /*handle_space*/0);
            dprintf(fd,
                ",\n"
                "  \"respfile\": \"%s\"",
                resp ? resp : e->resp_file);
            free((void*)resp);
        }
        dprintf(fd,"\n}\n");
    }
    free((void *)cmd);
}

static char const ** compilers = 0;

static int is_known_compiler(char const * cmd)
{
    // looking for compiler name
    // have to copy cmd since POSIX basename modifies input
    char * local_cmd = strdup(cmd);
    char * file = basename(local_cmd);
    int result = bear_strings_find(compilers, file);
    free(local_cmd);
    return result;
}

static void print_array(char const * const * const in)
{
    char const * const * it = in;
    for (; *it; ++it)
    {
        printf("  %s\n",*it);
    }
}

void bear_set_known_compilers(char const * csv)
{
    compilers = bear_strings_unfold(csv, ',');
}

void bear_print_known_compilers()
{
    printf("Known compilers:\n");
    print_array(compilers);
}
