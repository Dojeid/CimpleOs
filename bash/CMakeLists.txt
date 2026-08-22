cmake_minimum_required(VERSION 3.16)
project(bash C)

# Ensure bash headers are prepared
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONIOENCODING=utf-8 "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/prepare_bash.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
    RESULT_VARIABLE PREPARE_BASH_RES
)

set(BASH_INC_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/builtins"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
    "${CMAKE_SOURCE_DIR}/musl-1.2.6/include"
    "${CMAKE_SOURCE_DIR}/musl-1.2.6/arch/x86_64"
)

include_directories(${BASH_INC_DIRS})

# Top-level GNU Bash C Sources
set(BASH_CORE_SOURCES
    shell.c
    eval.c
    execute_cmd.c
    make_cmd.c
    copy_cmd.c
    dispose_cmd.c
    print_cmd.c
    redir.c
    subst.c
    variables.c
    general.c
    error.c
    findcmd.c
    hashcmd.c
    hashlib.c
    input.c
    list.c
    locale.c
    mailcheck.c
    pathexp.c
    pcomplete.c
    pcomplib.c
    stringlib.c
    test.c
    trap.c
    unwind_prot.c
    xmalloc.c
    y.tab.c
    syntax.c
    version.c
    flags.c
    nojobs.c
    braces.c
    bracecomp.c
    array.c
    arrayfunc.c
    assoc.c
    expr.c
    sig.c
    alias.c
    signames.c
)

# Builtin C Sources
set(BASH_BUILTIN_SOURCES
    builtins/common.c
    builtins/evalfile.c
    builtins/evalstring.c
    builtins/bashgetopt.c
    builtins/getopt.c
    builtins/getopts.c
    builtins/shopt.c
    builtins/echo.c
    builtins/cd.c
    builtins/eval.c
    builtins/exec.c
    builtins/exit.c
    builtins/help.c
    builtins/return.c
    builtins/set.c
    builtins/shift.c
    builtins/source.c
    builtins/test.c
    builtins/type.c
    builtins/alias.c
    builtins/command.c
    builtins/declare.c
    builtins/history.c
    builtins/pushd.c
    builtins/read.c
    builtins/umask.c
    builtins/setattr.c
    builtins/wait.c
    builtins/break.c
    builtins/mapfile.c
    builtins/kill.c
    builtins/builtins.c
)

# Helper Library C Sources (lib/sh, lib/glob, lib/tilde)
set(BASH_LIB_SOURCES
    lib/sh/casemod.c
    lib/sh/clktck.c
    lib/sh/clock.c
    lib/sh/compat.c
    lib/sh/eaccess.c
    lib/sh/mailstat.c
    lib/sh/fmtulong.c
    lib/sh/fmtullong.c
    lib/sh/fmtumax.c
    lib/sh/fnxform.c
    lib/sh/getcwd.c
    lib/sh/getenv.c
    lib/sh/gettimeofday.c
    lib/sh/itos.c
    lib/sh/makepath.c
    lib/sh/mbscmp.c
    lib/sh/mbschr.c
    lib/sh/mbscasecmp.c
    lib/sh/oslib.c
    lib/sh/pathcanon.c
    lib/sh/pathphys.c
    lib/sh/random.c
    lib/sh/shmatch.c
    lib/sh/shmbchar.c
    lib/sh/shquote.c
    lib/sh/shtty.c
    lib/sh/spell.c
    lib/sh/strcasecmp.c
    lib/sh/strcasestr.c
    lib/sh/strchrnul.c
    lib/sh/strdup.c
    lib/sh/strerror.c
    lib/sh/stringlist.c
    lib/sh/stringvec.c
    lib/sh/strlcpy.c
    lib/sh/strnlen.c
    lib/sh/strpbrk.c
    lib/sh/strscpy.c
    lib/sh/strtod.c
    lib/sh/strtol.c
    lib/sh/strtoll.c
    lib/sh/strtoul.c
    lib/sh/strtoull.c
    lib/sh/strtrans.c
    lib/sh/uconvert.c
    lib/sh/input_avail.c
    lib/sh/timers.c
    lib/sh/strvis.c
    lib/sh/ufuncs.c
    lib/sh/anonfile.c
    lib/sh/tmpfile.c
    lib/sh/setlinebuf.c
    lib/sh/netconn.c
    lib/sh/zgetline.c
    lib/sh/zmapfd.c
    lib/sh/zcatfd.c
    lib/sh/unicode.c
    lib/sh/utf8.c
    lib/sh/winsize.c
    lib/sh/zread.c
    lib/sh/zwrite.c
    lib/glob/glob.c
    lib/glob/strmatch.c
    lib/glob/smatch.c
    lib/glob/gmisc.c
    lib/glob/xmbsrtowcs.c
    lib/tilde/tilde.c
)

add_library(bash OBJECT
    ${BASH_CORE_SOURCES}
    ${BASH_BUILTIN_SOURCES}
    ${BASH_LIB_SOURCES}
)

target_include_directories(bash PUBLIC ${BASH_INC_DIRS})

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(bash PRIVATE
        -w
        -ffreestanding
        -fno-stack-protector
        -msse -msse2
        -DHAVE_CONFIG_H
        -DSHELL
    )
endif()
