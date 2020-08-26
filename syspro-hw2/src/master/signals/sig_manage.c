
#include "header.h"
#include "sig_actions.h"

#include "sig_manage.h"

// Flags that show whether a signal of said got_<type> arrived & awaits handling.
// 0 if no signal arrived, else 1.
static volatile sig_atomic_t got_intq, got_usr2, got_chld;

static sigset_t cmd_set;  // Mask with blocked signals during a command process

/* ========================================================================= */

static void handle_sig_usr2(int signo) {
  got_usr2 = 1;
}

static void handle_sig_int_quit(int signo) {
  got_intq = 1;
}

static void handle_sig_chld(int signo) {
  got_chld = 1;
}

/* ========================================================================= */

// Unblock signals INT, QUIT, CHLD, USR2 .
void signals_unblock(void) {
  sigprocmask(SIG_UNBLOCK, &cmd_set, NULL);
}

// Block signals INT, QUIT, CHLD, USR2 .
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

  got_intq = got_usr2 = got_chld = 0;  // No signals present

  memset(&act, 0, sizeof(struct sigaction));

  sigfillset(&act.sa_mask);  // Block all other signals while handling one
  
  act.sa_handler = handle_sig_usr2;
  sigaction(SIGUSR2, &act, NULL);  // Install signal handlers

  act.sa_handler = handle_sig_int_quit;
  sigaction(SIGINT,  &act, NULL);
  sigaction(SIGQUIT, &act, NULL);

  act.sa_handler = handle_sig_chld;
  sigaction(SIGCHLD, &act, NULL);

  sigemptyset(&cmd_set);  // Set signals blocked during a command process
  sigaddset(&cmd_set, SIGINT);
  sigaddset(&cmd_set, SIGQUIT);
  //sigaddset(&cmd_set, SIGCHLD);
  sigaddset(&cmd_set, SIGUSR2);

  sigemptyset(&full_set);
  sigprocmask(SIG_SETMASK, &full_set, NULL);  // Unblock every signal
}

/* ========================================================================= */

// Check if signals are pending & handle them.
void signals_check(void)
{
  if (got_intq)  // INT, QUIT
  {
    actions_quit(false, NULL, NULL, NULL, NULL, NULL);
    exit(EXIT_FAILURE);
  }

  if (got_usr2)
    actions_usr2(false, NULL);  // USR2

  if (got_chld)  // SIGCHLD
  {
    fprintf(stderr, "[ERROR] Non-fatal error: Child unexpectedly terminated.\
Creating a new child, current result might be unrealiable. Please repeat your last query, if given.\n");
    actions_child_term(false, NULL, NULL, NULL, NULL, NULL, NULL);
  }

  got_intq = got_usr2 = got_chld = 0;  // Reset available signals' bools.
}

/* ========================================================================= */
