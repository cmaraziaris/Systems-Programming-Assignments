
#include "header.h"

#include "io_files.h"
#include "glob_structs.h"
#include "signal_handling.h"

// Flags that show whether a signal of said got_<type> arrived & awaits handling.
// 0 if no signal is pending, else 1.
static volatile sig_atomic_t got_intquit;
static volatile sig_atomic_t got_usr1;

static sigset_t cmd_set;  // Mask with blocked signals during a command process

/* ========================================================================= */
static void handle_sig_int_quit(int signo) {
  got_intquit = 1;
}

static void handle_sig_usr1(int signo) {
  got_usr1 = 1;
}

/* ========================================================================= */

// Unblock signals INT, QUIT, USR1 .
void signals_unblock(void) {
  sigprocmask(SIG_UNBLOCK, &cmd_set, NULL);
}

// Block signals INT, QUIT, USR1 .
void signals_block(void) {
  sigprocmask(SIG_BLOCK, &cmd_set, NULL);
}

/* ========================================================================= */

// Configure singal handlers & masks.
void signals_config(void)
{
  sigset_t set;
  struct sigaction act;
  
  memset(&act, 0, sizeof(struct sigaction));

  sigfillset(&set);
  sigprocmask(SIG_SETMASK, &set, NULL);  // Block every signal

  got_intquit = got_usr1 = 0;  // Init flags

  sigfillset(&act.sa_mask);  // Block all other signals while handling one
  
  act.sa_handler = handle_sig_int_quit;
  sigaction(SIGINT,  &act, NULL);
  sigaction(SIGQUIT, &act, NULL);

  act.sa_handler = handle_sig_usr1;
  sigaction(SIGUSR1, &act, NULL);

  sigemptyset(&cmd_set);  // Set signals blocked during a command process
  sigaddset(&cmd_set, SIGINT);
  sigaddset(&cmd_set, SIGQUIT);
  sigaddset(&cmd_set, SIGUSR1);

  sigemptyset(&set);
  sigprocmask(SIG_SETMASK, &set, NULL); // Unblock every signal
}

/* ========================================================================= */

// Check if signals are pending & handle them.
void signals_check(void)
{
  if (got_intquit)
  {
    write_log_file();
    cleanup_structures();
    exit(1);
  }

  if (got_usr1) {
    read_dir_updates();
  }

  got_intquit = got_usr1 = 0; // Reset available signals' bools.
}

/* ========================================================================= */