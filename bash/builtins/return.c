#include "config.h"



#include <config.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include "../bashintl.h"

#include "../shell.h"
#include "../execute_cmd.h"
#include "common.h"
#include "bashgetopt.h"

/* If we are executing a user-defined function then exit with the value
   specified as an argument.  if no argument is given, then the last
   exit status is used. */
int
return_builtin (WORD_LIST *list)
{
  CHECK_HELPOPT (list);

  return_catch_value = get_exitstat (list);
  /* return is a special builtin, so non-interactive shells in posix mode
     exit on an invalid numeric argument. We have to do it this way because
     the return_catch targets aren't set up to deal with EXITSHELL, so we
     just jump_to_top_level directly. */
  if (interactive_shell == 0 && posixly_correct && executing_command_builtin == 0 && return_catch_value > EX_SHERRBASE)
    return (return_catch_value);

  if (return_catch_flag)
    sh_longjmp (return_catch, 1);
  else
    {
      builtin_error (_("can only `return' from a function or sourced script"));
      return (EX_USAGE);
    }
}