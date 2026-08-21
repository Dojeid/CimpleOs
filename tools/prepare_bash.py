#!/usr/bin/env python3
"""
tools/prepare_bash.py
Generates config.h, pathnames.h, signames.h, signames.c, syntax.c, and converts builtins/*.def to builtins/*.c for GNU Bash.
"""

import os
import sys
from pathlib import Path

def prepare_bash(root_dir=None):
    if root_dir is None:
        root_dir = Path(__file__).parent.parent.resolve()
    else:
        root_dir = Path(root_dir).resolve()

    bash_dir = root_dir / "bash"
    if not bash_dir.exists():
        print(f"[ERROR] bash directory not found at {bash_dir}")
        return False

    config_h = bash_dir / "config.h"
    pathnames_h = bash_dir / "pathnames.h"
    conftypes_h = bash_dir / "conftypes.h"
    signames_h = bash_dir / "signames.h"
    signames_c = bash_dir / "signames.c"
    syntax_c = bash_dir / "syntax.c"
    builtins_dir = bash_dir / "builtins"
    builtext_h = builtins_dir / "builtext.h"
    pipesize_h = builtins_dir / "pipesize.h"
    builtins_c = builtins_dir / "builtins.c"

    # 1. Generate config.h
    config_content = """#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <wchar.h>
#include <wctype.h>

typedef uint16_t bits16_t;
typedef uint32_t bits32_t;
typedef uint64_t bits64_t;
typedef uint32_t u_bits32_t;

#define main bash_main
#define signal_is_pending bash_signal_is_pending

#define HOSTTYPE "x86_64"
#define OSTYPE "falkon"
#define MACHTYPE "x86_64-falkon-elf"
#define GETGROUPS_T gid_t
#define BASH_SOURCE_FULLPATH_DEFAULT 0
#define EXTGLOB_DEFAULT 1
#define GLOBASCII_DEFAULT 0
#define LOCALEDIR "/usr/share/locale"
#define PACKAGE "bash"

#define DISTVERSION "5.2"
#define BUILDVERSION 1
#define SCCSVERSION "@(#)bash 5.2-release"
#define DEFAULT_COMPAT_LEVEL 52

#define SHELL 1
#define HAVE_UNISTD_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TIMES_H 1
#define HAVE_TIMES 1
#define HAVE_STRUCT_TMS 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_TERMIO_H 1
#define HAVE_DIRENT_H 1
#define HAVE_LOCALE_H 1
#define HAVE_LIMITS_H 1
#define HAVE_FCNTL_H 1
#define HAVE_PWD_H 1
#define HAVE_TIMEVAL 1
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_ISBLANK 1
#define HAVE_GETPID 1
#define HAVE_GETPPID 1
#define HAVE_GETCWD 1
#define HAVE_GETHOSTNAME 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_DPRINTF 1
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRSIGNAL 1
#define HAVE_STRFTIME 1
#define HAVE_STRSTR 1
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOUMAX 1
#define HAVE_STRTOIMAX 1
#define HAVE_MKFIFO 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_WINT_T 1
#define HAVE_SETLOCALE 1
#define HAVE_SELECT 1
#define HAVE_ISATTY 1
#define HAVE_ALLOCA 1
#define HAVE_ALLOCA_H 1
#define HAVE_MEMSET 1
#define HAVE_MEMMOVE 1
#define HAVE_BCOPY 1
#define HAVE_TCGETATTR 1
#define HAVE_TCSETATTR 1
#define HAVE_SIGINTERRUPT 1
#define HAVE_POSIX_SIGNALS 1
#define GETPGRP_VOID 1
#define RETSIGTYPE void
#define ALIAS 1
#define PUSH_POP_DIRECTORY 1
#define DYNAMIC_SCHEDULE 1
#define PROCESS_SUBSTITUTION 1
#define PROMPT_STRING_DECODE 1
#define SELECT_COMMAND 1
#define HELP_BUILTIN 1
#define ARRAY_VARS 1
#define DNODE_MUTEX 1
#define BRACE_EXPANSION 1
#define COMMAND_TIMING 1
#define EXTENDED_GLOB 1
#define COND_COMMAND 1
#define COND_REGEXP 1
#define ARITH_FOR_COMMAND 1
#define NETWORK_REDIRECTIONS 1
#define PROGRAMMABLE_COMPLETION 1
#define DEBUGGER 1

#define DEFAULT_PATH_VALUE "/bin:/usr/bin"
#define STANDARD_UTILS_PATH "/bin"

#include "config-top.h"
#include "config-bot.h"

extern int locale_utf8locale;

struct word_list;
extern void print_arith_command(struct word_list *);
extern void xtrace_print_arith_cmd(struct word_list *);

#endif /* _CONFIG_H_ */
"""
    config_h.write_text(config_content, encoding="utf-8")

    # 2. Generate pathnames.h
    pathnames_content = """#ifndef _PATHNAMES_H_
#define _PATHNAMES_H_

#define DEFAULT_PATH_VALUE "/bin:/usr/bin"
#define STANDARD_UTILS_PATH "/bin"
#define SYS_PROFILE "/etc/profile"
#define READLINE_INIT_FILE "~/.inputrc"

#endif /* _PATHNAMES_H_ */
"""
    pathnames_h.write_text(pathnames_content, encoding="utf-8")

    # 3. Generate conftypes.h
    conftypes_content = """#ifndef _CONFTYPES_H_
#define _CONFTYPES_H_
#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>
typedef uint16_t bits16_t;
typedef uint32_t bits32_t;
typedef uint64_t bits64_t;
typedef uint32_t u_bits32_t;
#define GETGROUPS_T gid_t
#define BASH_SOURCE_FULLPATH_DEFAULT 0
#define EXTGLOB_DEFAULT 1
#define GLOBASCII_DEFAULT 0
#endif
"""
    conftypes_h.write_text(conftypes_content, encoding="utf-8")

    # 4. Generate signames.h & signames.c
    signames_content = """#ifndef _SIGNAMES_H_
#define _SIGNAMES_H_

#define sys_nsig 32
#define NSIG 32

extern const char *const signal_names[NSIG];
static inline void initialize_signames(void) {}

#endif /* _SIGNAMES_H_ */
"""
    signames_h.write_text(signames_content, encoding="utf-8")

    signames_c_content = """/* Generated signames.c for GNU Bash */
#include "config.h"
#include "signames.h"

const char *const signal_names[NSIG] = {
    "EXIT", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "BUS",
    "FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM",
    "STKFLT", "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU", "URG",
    "XCPU", "XFSZ", "VTALRM", "PROF", "WINCH", "IO", "PWR", "SYS"
};
"""
    signames_c.write_text(signames_c_content, encoding="utf-8")

    # 5. Generate syntax.c
    syntax_content = """/* Generated syntax.c for GNU Bash */
#include "config.h"
#include "stdc.h"
#include "syntax.h"

int sh_syntabsiz = 256;

int sh_syntaxtab[256] = {
    /* 0..31 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, CBLANK|CSHBRK, CSHBRK, 0, 0, CSHBRK, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 32..63 */
    CBLANK|CSHBRK, 0, CQUOTE, 0, CSPECVAR, 0, CSHMETA|CSHBRK, CQUOTE,
    CSHBRK, CSHBRK, CGLOB, 0, 0, 0, 0, 0,
    0, CSPECVAR, CSPECVAR, CSPECVAR, CSPECVAR, 0, 0, 0,
    0, 0, 0, CSHMETA|CSHBRK, CSHMETA|CSHBRK, 0, CSHMETA|CSHBRK, CGLOB,
    /* 64..95 */
    CSPECVAR, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, CXGLOB, CXQUOTE, 0, 0, CSPECVAR,
    /* 96..127 */
    CBACKQ, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, CSHBRK, CSHMETA|CSHBRK, CSHBRK, 0, 0
};
"""
    syntax_c.write_text(syntax_content, encoding="utf-8")

    # 6. Convert builtins/*.def -> builtins/*.c starting AFTER $PRODUCES
    def_files = list(builtins_dir.glob("*.def"))
    for df in def_files:
        lines = df.read_text(encoding="utf-8", errors="ignore").splitlines()
        c_lines = ["#include \"config.h\""]
        past_produces = False
        in_doc = False
        for l in lines:
            if "$PRODUCES" in l:
                past_produces = True
                continue
            if not past_produces:
                continue
            if l.startswith('$SHORT_DOC'):
                in_doc = True
                continue
            elif l.startswith('$END'):
                in_doc = False
                continue
            elif l.startswith('$'):
                continue
            if in_doc:
                continue
            c_lines.append(l)
        target_c = df.with_suffix(".c")
        target_c.write_text('\n'.join(c_lines), encoding="utf-8")

    # 7. Generate builtins/builtext.h & builtins/pipesize.h & builtins/builtins.c
    builtext_content = """#ifndef _BUILTEXT_H_
#define _BUILTEXT_H_

struct word_list;

extern int cd_builtin(struct word_list *);
extern int pwd_builtin(struct word_list *);
extern int echo_builtin(struct word_list *);
extern int export_builtin(struct word_list *);
extern int alias_builtin(struct word_list *);
extern int unalias_builtin(struct word_list *);
extern int set_builtin(struct word_list *);
extern int unset_builtin(struct word_list *);
extern int help_builtin(struct word_list *);
extern int eval_builtin(struct word_list *);
extern int exec_builtin(struct word_list *);
extern int exit_builtin(struct word_list *);
extern int test_builtin(struct word_list *);
extern int type_builtin(struct word_list *);
extern int umask_builtin(struct word_list *);
extern int shift_builtin(struct word_list *);
extern int read_builtin(struct word_list *);
extern int history_builtin(struct word_list *);
extern int bind_builtin(struct word_list *);
extern int shopt_builtin(struct word_list *);
extern int command_builtin(struct word_list *);
extern int source_builtin(struct word_list *);
extern int mapfile_builtin(struct word_list *);
extern int return_builtin(struct word_list *);
extern int declare_builtin(struct word_list *);
extern int local_builtin(struct word_list *);
extern int typeset_builtin(struct word_list *);
extern int wait_builtin(struct word_list *);
extern int fg_builtin(struct word_list *);
extern int bg_builtin(struct word_list *);
extern int jobs_builtin(struct word_list *);
extern int kill_builtin(struct word_list *);
extern int getopts_builtin(struct word_list *);

#endif /* _BUILTEXT_H_ */
"""
    builtext_h.write_text(builtext_content, encoding="utf-8")

    pipesize_content = """#ifndef _PIPESIZE_H_
#define _PIPESIZE_H_
#define PIPESIZE 4096
#endif
"""
    pipesize_h.write_text(pipesize_content, encoding="utf-8")

    builtins_c_content = """/* Generated builtins.c table */
#include "config.h"
#include "shell.h"
#include "builtins.h"
#include "builtext.h"

struct builtin *current_builtin = NULL;

struct builtin static_shell_builtins[] = {
    { "cd", cd_builtin, BUILTIN_ENABLED, NULL, "Change working directory", NULL },
    { "pwd", pwd_builtin, BUILTIN_ENABLED, NULL, "Print working directory", NULL },
    { "echo", echo_builtin, BUILTIN_ENABLED, NULL, "Print arguments", NULL },
    { "export", export_builtin, BUILTIN_ENABLED, NULL, "Set environment variable", NULL },
    { "alias", alias_builtin, BUILTIN_ENABLED, NULL, "Define command alias", NULL },
    { "set", set_builtin, BUILTIN_ENABLED, NULL, "Set shell options", NULL },
    { "unset", unset_builtin, BUILTIN_ENABLED, NULL, "Unset variable or function", NULL },
    { "help", help_builtin, BUILTIN_ENABLED, NULL, "Display shell builtin help", NULL },
    { "exit", exit_builtin, BUILTIN_ENABLED, NULL, "Exit shell", NULL },
    { NULL, NULL, 0, NULL, NULL, NULL }
};

struct builtin *shell_builtins = static_shell_builtins;
int num_shell_builtins = 9;
"""
    builtins_c.write_text(builtins_c_content, encoding="utf-8")

    print("[bash] GNU Bash configuration headers, builtins/*.c, and syntax.c prepared successfully!")
    return True

if __name__ == "__main__":
    success = prepare_bash()
    sys.exit(0 if success else 1)
