
#include "header.h"
#include "ipc.h"

/* A message is composed from the following components:
 * <opcode> - < # bytes of the actual message> - <actual message>
 * Size in bytes: [1 B]- ["MAX_DIGITS" B] - [<variable_bytes> B]
 * 
 * <header> : <opcode> + <#bytes>
 * <body>   : <actual message>
 */

 // Up to 2.147.483.647 (INT_MAX) bytes can be transmitted at once (for the 10 value)
#define MAX_DIGITS 10
#define HEAD_BYTES (1 + MAX_DIGITS) // Bytes reserved for the <header> part of the message


/*========================================================================== */

int get_opcode(char *message) {
  return message[0];
}

// Return the actual message transmitted.
char *decode_message(char *message) {
  return message + HEAD_BYTES;
}

/*========================================================================== */

// Sends <message> to file descriptor <fd> for operation <opcode>.
int send_message(int fd, int opcode, char *message, int buf_size)
{
  int msg_len = strlen(message);
  char *full_msg = malloc((msg_len + HEAD_BYTES + 1) * sizeof(char)); // <body> + <header> + \0

  // Compose the full message
  full_msg[0] = opcode;
  snprintf(full_msg+1, MAX_DIGITS+1, "%0*d", MAX_DIGITS, msg_len);
  snprintf(full_msg+HEAD_BYTES, msg_len+1, "%s", message);

  msg_len = strlen(full_msg);
  char buf[buf_size];

  int total_sent = 0;
  while (total_sent < msg_len)
  {
    int diff = msg_len - total_sent;  // We might need to write less bytes
    int write_num = (diff < buf_size) ? diff : buf_size;  // than <buf_size>

    strncpy(buf, full_msg + total_sent, write_num);  // Copy bytes from message to buffer

    int written = write(fd, buf, write_num);
    if (written < 0) {
      perror("write @ send_message");
      exit(1);
    }

    total_sent += written;
  }

  free(full_msg);
  return 0;
}

/*========================================================================== */

static int read_wrapper(int fd, char *buf, int read_num, bool ignore_sig);
static int read_n_bytes(char *full_msg, int n, int fd, char *buf, int buf_size, bool ignore_sig);

// Reads a message from <fd> and returns it, allocated on the heap.
// Returns NULL on failure (signal interrupt).
char *read_message(int fd, int buf_size)
{
  char buf[buf_size];
  memset(buf, 0, buf_size);

  char head_msg[HEAD_BYTES+1];  // <header> + \0

  if (read_n_bytes(head_msg, 1, fd, buf, buf_size, false) == -1)  // Read <opcode>
    return NULL;

  // Read rest of <header>
  read_n_bytes(head_msg + 1, MAX_DIGITS, fd, buf, buf_size, true);

  head_msg[HEAD_BYTES] = '\0';

  int bytes_to_read = atoi(head_msg+1); // Get the # bytes to read

  char *full_msg = calloc((HEAD_BYTES + bytes_to_read + 1), sizeof(char));
  strcpy(full_msg, head_msg);

  read_n_bytes(full_msg + HEAD_BYTES, bytes_to_read, fd, buf, buf_size, true);

  full_msg[HEAD_BYTES + bytes_to_read] = '\0';
  return full_msg;
}


// Reads exactly <n> bytes from <fd> and store them in <full_msg>.
// If <ignore_sig> is *false*, returns -1 on signal interrupt.
static int read_n_bytes(char *full_msg, int n, int fd, char *buf, int buf_size, bool ignore_sig)
{
  int diff, read_num, data_read;
  int chars_read = 0;  // Chars read so far

  while (chars_read < n)
  {
    diff = n - chars_read;  // If less bytes than <buf_size> remain, read less.
    read_num = (diff < buf_size) ? diff : buf_size;

    data_read = read_wrapper(fd, buf, read_num, ignore_sig);
    if (data_read == -1)
      return -1;
    if (data_read == 0)
      continue;

    strncpy(full_msg + chars_read, buf, read_num);
    chars_read += data_read;
  }

  return chars_read;
}


// `read` syscall wrapper
// Returns number of bytes read.
// If <ignore_sig> is *false*, returns -1 on signal interrupt.
static int read_wrapper(int fd, char *buf, int read_num, bool ignore_sig)
{
  int data_read = read(fd, buf, read_num);
  if (data_read == -1)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return 0;         // We ignore EAGAIN or EWOULDBLOCK errors by default.

    if (errno == EINTR)  // Signal interrupt
    {
      if (ignore_sig == false)
        return -1;
      else
        return 0;  // Ignore signal
    }

    perror("read @ read_message");
    exit(1);
  }

  return data_read;
}

/*========================================================================== */
