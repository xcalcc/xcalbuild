// This file is distributed under MIT-LICENSE. See COPYING for details.

#include "config.h"

#include "stringarray.h"
#include "environ.h"
#include "protocol.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <dlfcn.h>

#if defined HAVE_POSIX_SPAWN || defined HAVE_POSIX_SPAWNP
#include <spawn.h>
#endif

static int already_reported = 0;

static void report_call(char const * fun, char const * const argv[]);
static void report_failed_call(char const * fun, int result, int report_state);

#ifdef HAVE_EXECVE
static int call_execve(const char * path, char * const argv[], char * const envp[]);
#endif
#ifdef HAVE_EXECVP
static int call_execvp(const char * file, char * const argv[]);
#endif
#ifdef HAVE_EXECVPE
static int call_execvpe(const char * file, char * const argv[], char * const envp[]);
#endif
#ifdef HAVE_EXECVP2
static int call_execvP(const char * file, const char * search_path, char * const argv[]);
#endif
#ifdef HAVE_POSIX_SPAWN
// https://man7.org/linux/man-pages/man3/posix_spawn.3.html
static int call_posix_spawn(
    pid_t *pid, const char *path,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[], char *const envp[]);
#endif
#ifdef HAVE_POSIX_SPAWNP
static int call_posix_spawnp(
    pid_t *pid, const char *file,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[], char *const envp[]);
#endif
#ifdef HAVE_VFORK
pid_t vfork(void)
{
    return fork();
}
#endif

#ifdef HAVE_EXECVE
int execve(const char * path, char * const argv[], char * const envp[])
{
    int const report_state = already_reported;

    report_call("execve", (char const * const *)argv);
    int const result = call_execve(path, argv, envp);
    report_failed_call("execve", result, report_state);

    return result;
}
#endif

#ifdef HAVE_EXECV
# ifndef HAVE_EXECVE
#  error can not implement execv without execve
# endif
int execv(const char * path, char * const argv[])
{
    int const report_state = already_reported;

    report_call("execv", (char const * const *)argv);
    int const result = call_execve(path, argv, bear_get_environ());
    report_failed_call("execv", result, report_state);
    return result;
}
#endif

#ifdef HAVE_EXECVPE
int execvpe(const char * file, char * const argv[], char * const envp[])
{
    int const report_state = already_reported;

    report_call("execvpe", (char const * const *)argv);
    int const result = call_execvpe(file, argv, envp);
    report_failed_call("execvpe", result, report_state);

    return result;
}
#endif

#ifdef HAVE_EXECVP
int execvp(const char * file, char * const argv[])
{
    int const report_state = already_reported;

    report_call("execvp", (char const * const *)argv);
    int const result = call_execvp(file, argv);
    report_failed_call("execvp", result, report_state);
    return result;
}
#endif

#ifdef HAVE_EXECVP2
int execvP(const char * file, const char * search_path, char * const argv[])
{
    int const report_state = already_reported;

    report_call("execvP", (char const * const *)argv);
    int const result = call_execvP(file, search_path, argv);
    report_failed_call("execvP", result, report_state);
    return result;
}
#endif

#ifdef HAVE_EXECL
# ifndef HAVE_EXECVE
#  error can not implement execl without execve
# endif
int execl(const char * path, const char * arg, ...)
{
    va_list args;
    va_start(args, arg);
    char const ** argv = bear_strings_build(arg, args);
    va_end(args);

    report_call("execl", (char const * const *)argv);
    int const result = call_execve(path, (char * const *)argv, bear_get_environ());
    report_failed_call("execl", result, 0);
    bear_strings_release(argv);
    return result;
}
#endif

#ifdef HAVE_EXECLP
# ifndef HAVE_EXECVP
#  error can not implement execlp without execvp
# endif
int execlp(const char * file, const char * arg, ...)
{
    va_list args;
    va_start(args, arg);
    char const ** argv = bear_strings_build(arg, args);
    va_end(args);

    report_call("execlp", (char const * const *)argv);
    int const result = call_execvp(file, (char * const *)argv);
    report_failed_call("execlp", result, 0);
    bear_strings_release(argv);
    return result;
}
#endif

#ifdef HAVE_EXECLE
# ifndef HAVE_EXECVE
#  error can not implement execle without execve
# endif
// int execle(const char *path, const char *arg, ..., char * const envp[]);
int execle(const char * path, const char * arg, ...)
{
    va_list args;
    va_start(args, arg);
    char const ** argv = bear_strings_build(arg, args);
    char const ** envp = va_arg(args, char const **);
    va_end(args);

    report_call("execle", (char const * const *)argv);
    int const result = call_execve(path, (char * const *)argv, (char * const *)envp);
    report_failed_call("execle", result, 0);
    bear_strings_release(argv);
    return result;
}
#endif

#ifdef HAVE_POSIX_SPAWN
int posix_spawn(
    pid_t *pid, const char *path,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[], char *const envp[]
) {
    report_call("posix_spawn", (char const *const *)argv);
    return call_posix_spawn(pid, path, file_actions, attrp, argv, envp);
}
#endif

#ifdef HAVE_POSIX_SPAWNP
int posix_spawnp(
    pid_t *pid, const char *file,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[], char *const envp[]
) {
    report_call("posix_spawnp", (char const *const *)argv);
    return call_posix_spawnp(pid, file, file_actions, attrp, argv, envp);
}
#endif

#ifdef HAVE_EXECVE
static int call_execve(const char * path, char * const argv[], char * const envp[])
{
    int (*fp)(const char *, char * const *, char * const *) = 0;
    if (0 == (fp = dlsym(RTLD_NEXT, "execve")))
    {
        perror("xcalbuild: dlsym");
        exit(EXIT_FAILURE);
    }

    char const ** menvp = bear_strings_copy((char const * *)envp);
    menvp = bear_env_insert(menvp, ENV_PRELOAD, getenv(ENV_PRELOAD));
    menvp = bear_env_insert(menvp, ENV_LIBRARY_PATH, getenv(ENV_LIBRARY_PATH));
    menvp = bear_env_insert(menvp, ENV_OUTPUT, getenv(ENV_OUTPUT));
#ifdef ENV_FLAT
    menvp = bear_env_insert(menvp, ENV_FLAT, getenv(ENV_FLAT));
#endif
    int const result = (*fp)(path, argv, (char * const *)menvp);
    bear_strings_release(menvp);
    return result;
}
#endif

#ifdef HAVE_EXECVPE
static int call_execvpe(const char * file, char * const argv[], char * const envp[])
{
    int (*fp)(const char *, char * const *, char * const *) = 0;
    if (0 == (fp = dlsym(RTLD_NEXT, "execvpe")))
    {
        perror("xcalbuild: dlsym");
        exit(EXIT_FAILURE);
    }

    char const ** menvp = bear_strings_copy((char const * *)envp);
    menvp = bear_env_insert(menvp, ENV_PRELOAD, getenv(ENV_PRELOAD));
    menvp = bear_env_insert(menvp, ENV_LIBRARY_PATH, getenv(ENV_LIBRARY_PATH));
    menvp = bear_env_insert(menvp, ENV_OUTPUT, getenv(ENV_OUTPUT));
#ifdef ENV_FLAT
    menvp = bear_env_insert(menvp, ENV_FLAT, getenv(ENV_FLAT));
#endif
    int const result = (*fp)(file, argv, (char * const *)menvp);
    bear_strings_release(menvp);
    return result;
}
#endif

#ifdef HAVE_EXECVP
static int call_execvp(const char * file, char * const argv[])
{
    int (*fp)(const char * file, char * const argv[]) = 0;
    if (0 == (fp = dlsym(RTLD_NEXT, "execvp")))
    {
        perror("xcalbuild: dlsym");
        exit(EXIT_FAILURE);
    }

    return (*fp)(file, argv);
}
#endif

#ifdef HAVE_EXECVP2
static int call_execvP(const char * file, const char * search_path, char * const argv[])
{
    int (*fp)(const char *, const char *, char * const *) = 0;
    if (0 == (fp = dlsym(RTLD_NEXT, "execvP")))
    {
        perror("xcalbuild: dlsym");
        exit(EXIT_FAILURE);
    }

    return (*fp)(file, search_path, argv);
}
#endif

#ifdef HAVE_POSIX_SPAWN
static int call_posix_spawn(
    pid_t *pid, const char *path,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[], char *const envp[]
) {
    int (*fp)(
        pid_t *pid, const char *path,
        const posix_spawn_file_actions_t *file_actions,
        const posix_spawnattr_t *attrp,
        char *const argv[], char *const envp[]
    ) = 0;

    if (0 == (fp = dlsym(RTLD_NEXT, "posix_spawn")))
    {
        perror("xcalbuild: dlsym");
        exit(EXIT_FAILURE);
    }

    char const ** menvp = bear_strings_copy((char const * *)envp);
    menvp = bear_env_insert(menvp, ENV_PRELOAD, getenv(ENV_PRELOAD));
    menvp = bear_env_insert(menvp, ENV_LIBRARY_PATH, getenv(ENV_LIBRARY_PATH));
    menvp = bear_env_insert(menvp, ENV_OUTPUT, getenv(ENV_OUTPUT));
#ifdef ENV_FLAT
    menvp = bear_env_insert(menvp, ENV_FLAT, getenv(ENV_FLAT));
#endif
    int const result = (*fp)(pid, path, file_actions, attrp, argv, (char *const *)menvp);
    bear_strings_release(menvp);
    return result;
}
#endif

#ifdef HAVE_POSIX_SPAWNP
static int call_posix_spawnp(
    pid_t *pid, const char *file,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[], char *const envp[]
) {
    int (*fp)(
        pid_t *pid, const char *file,
        const posix_spawn_file_actions_t *file_actions,
        const posix_spawnattr_t *attrp,
        char *const argv[], char *const envp[]
    ) = 0;

    if (0 == (fp = dlsym(RTLD_NEXT, "posix_spawnp")))
    {
        perror("xcalbuild: dlsym");
        exit(EXIT_FAILURE);
    }

    char const ** menvp = bear_strings_copy((char const * *)envp);
    menvp = bear_env_insert(menvp, ENV_PRELOAD, getenv(ENV_PRELOAD));
    menvp = bear_env_insert(menvp, ENV_LIBRARY_PATH, getenv(ENV_LIBRARY_PATH));
    menvp = bear_env_insert(menvp, ENV_OUTPUT, getenv(ENV_OUTPUT));
#ifdef ENV_FLAT
    menvp = bear_env_insert(menvp, ENV_FLAT, getenv(ENV_FLAT));
#endif
    int const result = (*fp)(pid, file, file_actions, attrp, argv, (char *const *)menvp);
    bear_strings_release(menvp);
    return result;
}
#endif

typedef void (*send_message)(char const * socket, struct bear_message const *);

static char const *read_resp_file(char const *resp_file) {
    if(access(resp_file, R_OK ) == -1) {
        return NULL;
    }

    int fd=open(resp_file, O_RDONLY);
    off_t size = lseek(fd, 0, SEEK_END);
    if(size == (off_t)-1) {
        perror("xcalbuild: resp file size error");
        return NULL;
    }
    lseek(fd, 0, SEEK_SET);

    char *buff = malloc(size + 1);
    if(buff == 0) {
        perror("xcalbuild: malloc");
        return NULL;
    }
    off_t actual_size = read(fd, buff, size);
    if(actual_size != size) {
        perror("xcalbuild: resp file read error, size mismatched");
        return NULL;
    }
    // The response file not necessarily ends with 0.
    // Put an end of string marker here just in case.
    buff[size] = 0;

    return buff;
}

static void report(send_message fp, char const * socket, char const * fun, char const * const argv[])
{
    size_t const length = bear_strings_length(argv);
    size_t it = 0;
    char const *resp_file = NULL;
    for (; it < length; ++it)
    {
        if(argv[it][0] == '@') {
            resp_file = read_resp_file(&argv[it][1]);
            break;
        }
    }

    struct bear_message const msg =
    {
        getpid(),
        getppid(),
        fun,
        getcwd(NULL, 0),
        resp_file,
        (char const **)argv
    };
    (*fp)(socket, &msg);
    free((void *)msg.cwd);
    if(resp_file) {
        free((void*)resp_file);
    }
}

static void report_call(char const * fun, char const * const argv[])
{
    if (already_reported)
        return;
    already_reported = 1;

    char * const socket = getenv(ENV_OUTPUT);
    if (0 == socket)
    {
        perror("xcalbuild: getenv");
        exit(EXIT_FAILURE);
    }

    return report(bear_send_message, socket, fun, argv);
}

static void report_failed_call(char const * fun, int result_code, int report_state)
{
    if (!report_state)
        already_reported = 0;
}
