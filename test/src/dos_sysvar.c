/* A child started with SystemTagList() gets a copy of the parent's local
   variables (NP_CopyVars default): it must see them, and changes it makes
   must not reach the parent. With NP_CopyVars FALSE it must not see them.

   The C library's environment calls are checked too where the runtime has
   them and vamos can run them, since local variables are how the
   environment reaches a child process on AmigaOS:
     libnix  getenv, setenv, putenv, unsetenv, all mirrored to local vars
     vbcc    getenv only (GetVar)
     SAS/C   none: its getenv and putenv use the global ENV: files, not
             local variables, and vamos has no ENV:
     AROS    none: getenv is in stdc.library and setenv/putenv/unsetenv in
             posixc.library, neither of which vamos provides, and the
             static libc we link against carries none of them

   usage: dos_sysvar <own_path>      (parent)
          dos_sysvar <own_path> child (started by the parent)
*/
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/var.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <string.h>
#include <stdlib.h>

#if defined(__libnix__) || defined(__VBCC__)
#define HAVE_GETENV 1
#endif
#ifdef __libnix__
#define HAVE_PUTENV 1
#define HAVE_SETENV 1
#endif

#define VAR   "VAMOSTEST"
#define FLAGS (GVF_LOCAL_ONLY | LV_VAR)
#define STACK 65536

int main(int argc, char *argv[])
{
  char buf[64];
  char cmd[256];
  struct TagItem tags[] = {
    {NP_StackSize, STACK},
    {TAG_DONE, 0}
  };
  struct TagItem nocopy_tags[] = {
    {NP_StackSize, STACK},
    {NP_CopyVars, FALSE},
    {TAG_DONE, 0}
  };
  LONG rc;

  if(argc == 3 && strcmp(argv[2], "child") == 0) {
    struct Task *task = FindTask(NULL);
    ULONG stack = (ULONG)task->tc_SPUpper - (ULONG)task->tc_SPLower;

    /* the parent asked for a bigger stack than the default */
    if(stack < STACK) {
      Printf("child: stack is %lu bytes\n", stack);
      return 7;
    }
    if(GetVar(VAR, buf, sizeof(buf), FLAGS) < 0) {
      Printf("child: no " VAR "\n");
      return 2;
    }
    if(strcmp(buf, "parent") != 0) {
      Printf("child: " VAR " is '%s'\n", (ULONG)buf);
      return 3;
    }
#ifdef HAVE_GETENV
    if(getenv(VAR) == NULL || strcmp(getenv(VAR), "parent") != 0) {
      Printf("child: getenv(" VAR ") gives '%s'\n", (ULONG)getenv(VAR));
      return 6;
    }
#endif
    SetVar(VAR, "child", -1, FLAGS);
    return 0;
  }

  if(argc != 2) {
    Printf("Usage: %s <own_path>\n", (ULONG)argv[0]);
    return 1;
  }

#ifdef HAVE_PUTENV
  putenv(VAR "=parent");
#else
  SetVar(VAR, "parent", -1, FLAGS);
#endif

  strcpy(cmd, argv[1]);
  strcat(cmd, " ");
  strcat(cmd, argv[1]);
  strcat(cmd, " child");
  rc = SystemTagList(cmd, tags);
  if(rc != 0) {
    Printf("SystemTagList returned: %ld\n", rc);
    return rc;
  }

  if(GetVar(VAR, buf, sizeof(buf), FLAGS) < 0 || strcmp(buf, "parent") != 0) {
    Printf("parent: " VAR " changed to '%s'\n", (ULONG)buf);
    return 4;
  }

  /* without NP_CopyVars the child must not see the variable: rc 2 */
  rc = SystemTagList(cmd, nocopy_tags);
  if(rc != 2) {
    Printf("SystemTagList without NP_CopyVars returned: %ld\n", rc);
    return 5;
  }

#ifdef HAVE_SETENV
  setenv(VAR, "again", 1);
  if(GetVar(VAR, buf, sizeof(buf), FLAGS) < 0 || strcmp(buf, "again") != 0) {
    Printf("parent: setenv did not reach " VAR ": '%s'\n", (ULONG)buf);
    return 7;
  }
  unsetenv(VAR);
  if(FindVar(VAR, LV_VAR) != NULL) {
    Printf("parent: unsetenv left " VAR "\n");
    return 8;
  }
#else
  DeleteVar(VAR, FLAGS);
#endif
  return 0;
}
