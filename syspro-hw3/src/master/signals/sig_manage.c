
#include "header.h"
#include "sig_actions.h"

#include "sig_manage.h"

// Flags that show whether a signal of said got_<type> arrived & awaits handling.
// 0 if no signal arrived, else 1.
static volatile sig_atomic_t got_intq, got_chld;

static sigset_t cmd_set;  // Mask with blocked signals during setup

/* ========================================================================= */

static void handle_sig_int_quit(int signo) {
  got_intq = signo;
}

static void handle_sig_chld(int signo) {
  got_chld = signo;
}

/* ========================================================================= */

// Check if signals are pending & handle them.
void signals_check(void)
{
  if (got_intq)  // INT, QUIT
  {
    actions_quit(false, NULL, NULL);
    exit(0);
  }

  if (got_chld)  // SIGCHLD
  {
    fprintf(stderr, "1 worker died. Replacing it...\n");
    actions_child_term(false, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    got_chld = 0;
  }
}

/* ========================================================================= */

// Unblock signals INT, QUIT, CHLD.
void signals_unblock(void) {
  sigprocmask(SIG_UNBLOCK, &cmd_set, NULL);
}

// Block signals INT, QUIT, CHLD.
void signals_block(void) {
  sigprocmask(SIG_BLOCK, &cmd_set, NULL);
}

/* ========================================================================= */

// Configure signal handlers, signal masks & availability bools.
void signals_config(void)
{
  sigset_t full_set;
  struct sigaction act;
  
  sigfillset(&full_set);
  sigprocmask(SIG_SETMASK, &full_set, NULL);  // Block every signal

  got_intq = got_chld = 0;  // No signals present

  memset(&act, 0, sizeof(struct sigaction));

  sigfillset(&act.sa_mask);  // Block all other signals while handling one
  
  act.sa_handler = handle_sig_int_quit;  // Install signal handlers
  sigaction(SIGINT,  &act, NULL);
  sigaction(SIGQUIT, &act, NULL);

  act.sa_handler = handle_sig_chld;
  sigaction(SIGCHLD, &act, NULL);

  sigemptyset(&cmd_set);  // Set signals blocked during setup
  sigaddset(&cmd_set, SIGINT);
  sigaddset(&cmd_set, SIGQUIT);
  sigaddset(&cmd_set, SIGCHLD);

  sigemptyset(&full_set);
  sigprocmask(SIG_SETMASK, &full_set, NULL);  // Unblock every signal
}

/* ========================================================================= */