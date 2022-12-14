// This file is distributed under MIT-LICENSE. See COPYING for details.

#include "protocol.h"
#include "stringarray.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


static size_t init_socket(char const * file, struct sockaddr_un * addr);

#ifdef SERVER
static ssize_t socket_read(int fd, void * buf, size_t nbyte)
{
    ssize_t sum = 0;
    while (sum != nbyte)
    {
        ssize_t const cur = read(fd, buf + sum, nbyte - sum);
        if (-1 == cur)
        {
            return cur;
        }
        sum += cur;
    }
    return sum;
}

static pid_t read_pid(int fd)
{
    pid_t result = 0;
    if (-1 == socket_read(fd, (void *)&result, sizeof(pid_t)))
    {
        perror("xcalbuild: read pid");
        exit(EXIT_FAILURE);
    }
    return result;
}

static char const * read_string(int fd)
{
    uint32_t length = 0;
    if (-1 == socket_read(fd, (void *)&length, sizeof(uint32_t)))
    {
        perror("xcalbuild: read string length");
        exit(EXIT_FAILURE);
    }
    char * result = malloc((length + 1) * sizeof(char));
    if (0 == result)
    {
        perror("xcalbuild: malloc");
        exit(EXIT_FAILURE);
    }
    if (length > 0)
    {
        if (-1 == socket_read(fd, (void *)result, length))
        {
            perror("xcalbuild: read string value");
            exit(EXIT_FAILURE);
        }
    }
    result[length] = '\0';
    return result;
}

static char const * * read_string_array(int fd)
{
    uint32_t length = 0;
    if (-1 == socket_read(fd, (void *)&length, sizeof(uint32_t)))
    {
        perror("xcalbuild: read string array length");
        exit(EXIT_FAILURE);
    }
    char const * * result =
        (char const * *)malloc((length + 1) * sizeof(char const *));
    if (0 == result)
    {
        perror("xcalbuild: malloc");
        exit(EXIT_FAILURE);
    }
    size_t it = 0;
    for (; it < length; ++it)
    {
        result[it] = read_string(fd);
    }
    result[length] = 0;
    return result;
}

void bear_read_message(int fd, struct bear_message * e)
{
    e->pid = read_pid(fd);
    e->ppid = read_pid(fd);
    e->fun = read_string(fd);
    e->cwd = read_string(fd);
    e->resp_file = read_string(fd);
    e->cmd = read_string_array(fd);
}

void bear_free_message(struct bear_message * e)
{
    e->pid = 0;
    e->ppid = 0;
    free((void *)e->fun);
    e->fun = 0;
    free((void *)e->cwd);
    e->cwd = 0;
    if(e->resp_file) {
        free((void *)e->resp_file);
        e->resp_file = 0;
    }
    bear_strings_release(e->cmd);
    e->cmd = 0;
}

int bear_create_unix_socket(char const * file)
{
    struct sockaddr_un addr;
    int s = init_socket(file, &addr);
    if (-1 == bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)))
    {
        perror("xcalbuild: bind");
        exit(EXIT_FAILURE);
    }
    if (-1 == listen(s, 0))
    {
        perror("xcalbuild: listen");
        exit(EXIT_FAILURE);
    }
    return s;
}

int bear_accept_message(int s, struct bear_message * msg)
{
    int connection = accept(s, 0, 0);
    if (-1 != connection)
    {
        bear_read_message(connection, msg);
        close(connection);
        return 1;
    }
    return 0;
}
#endif

#ifdef CLIENT
static ssize_t socket_write(int fd, const void *buf, size_t nbyte)
{
    ssize_t sum = 0;
    while (sum != nbyte)
    {
        ssize_t const cur = write(fd, buf + sum, nbyte - sum);
        if (-1 == cur)
        {
            return cur;
        }
        sum += cur;
    }
    return sum;
}

static void write_pid(int fd, pid_t pid)
{
    socket_write(fd, (void const *)&pid, sizeof(pid_t));
}

static void write_string(int fd, char const * message)
{
    uint32_t const length = (message) ? strlen(message) : 0;
    socket_write(fd, (void const *)&length, sizeof(uint32_t));
    if (length > 0)
    {
        socket_write(fd, (void const *)message, length);
    }
}

static void write_string_array(int fd, char const * const * message)
{
    uint32_t const length = bear_strings_length(message);
    socket_write(fd, (void const *)&length, sizeof(uint32_t));
    size_t it = 0;
    for (; it < length; ++it)
    {
        write_string(fd, message[it]);
    }
}

void bear_write_message(int fd, struct bear_message const * e)
{
    write_pid(fd, e->pid);
    write_pid(fd, e->ppid);
    write_string(fd, e->fun);
    write_string(fd, e->cwd);
    write_string(fd, e->resp_file);
    write_string_array(fd, e->cmd);
}

void bear_send_message(char const * file, struct bear_message const * msg)
{
    struct sockaddr_un addr;
    int s = init_socket(file, &addr);
    if (-1 == connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)))
    {
        perror("xcalbuild: connect");
        exit(EXIT_FAILURE);
    }
    bear_write_message(s, msg);
    close(s);
}
#endif

static size_t init_socket(char const * file, struct sockaddr_un * addr)
{
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == s)
    {
        perror("xcalbuild: socket");
        exit(EXIT_FAILURE);
    }
    memset((void *)addr, 0, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strncpy(addr->sun_path, file, sizeof(addr->sun_path) - 1);
    return s;
}
