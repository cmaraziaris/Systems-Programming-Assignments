
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

void destroy_message(struct message *received) {
  free(received->body);
}

/*========================================================================== */

// Sends <message> to file descriptor <fd> for operation <opcode>.
void send_message(int fd, int opcode, char *message, int buf_size)
{
  int msg_len = strlen(message);
  char *full_msg = malloc((msg_len + HEAD_BYTES + 1) * sizeof(char)); // <body> + <header> + \0

  // Compose the full message
  full_msg[0] = opcode;
  snprintf(full_msg+1, MAX_DIGITS+1, "%0*d", MAX_DIGITS, msg_len);
  snprintf(full_msg+HEAD_BYTES, msg_len+1, "%s", message);

  msg_len = strlen(full_msg);
  char buf[buf_size];

  int total_sent = 0, diff, write_num, written;
  while (total_sent < msg_len)
  {
    diff = msg_len - total_sent;  // We might need to write less bytes
    write_num = (diff < buf_size) ? diff : buf_size;  // than <buf_size>
    strncpy(buf, full_msg + total_sent, write_num);   // Copy bytes from message to buffer

    written = write(fd, buf, write_num);
    if (written < 0) {
      perror("write @ send_message");
      exit(1);
    }

    total_sent += written;
  }

  free(full_msg);
}

/*========================================================================== */

static int read_wrapper(int fd, char *buf, int read_num);
static int read_n_bytes(char *full_msg, int n, int fd, char *buf, int buf_size);

// Reads a message from <fd> and fills the struct message <received>.
// Returns 1 on failure (signal interrupt), 0 on success.
int read_message(struct message *received, int fd, int buf_size)
{
  char buf[buf_size];
  memset(buf, 0, buf_size);

  char head_msg[HEAD_BYTES+1];  // <header> + \0

  if (read_n_bytes(head_msg, 1, fd, buf, buf_size) == -1)  // Read <opcode>
    return 1;

  // Read rest of <header>
  if (read_n_bytes(head_msg + 1, MAX_DIGITS, fd, buf, buf_size) == -1)
    return 1;
  head_msg[HEAD_BYTES] = '\0';

  int bytes_to_read = atoi(head_msg+1); // Get the # bytes to read

  char *actual_msg = calloc((bytes_to_read + 1), sizeof(char));
  if (read_n_bytes(actual_msg, bytes_to_read, fd, buf, buf_size) == -1)
    return 1;

  if (received != NULL)
  {
    received->opcode = head_msg[0];
    received->body = actual_msg;
  }
  else
    free(actual_msg);

  return 0;
}


// Reads exactly <n> bytes from <fd> and store them in <full_msg>.
// Returns -1 on failure.
static int read_n_bytes(char *full_msg, int n, int fd, char *buf, int buf_size)
{
  int diff, read_num, data_read;
  int chars_read = 0;  // Chars read so far

  while (chars_read < n)
  {
    diff = n - chars_read;  // If less bytes than <buf_size> remain, read less.
    read_num = (diff < buf_size) ? diff : buf_size;

    data_read = read_wrapper(fd, buf, read_num);
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
// Returns number of bytes read or -1 on failure (ECONNRESET or EINTR).
static int read_wrapper(int fd, char *buf, int read_num)
{
  int data_read = read(fd, buf, read_num);
  if (data_read == -1)
  {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return 0;         // We ignore EAGAIN or EWOULDBLOCK errors by default.

    if (errno == ECONNRESET || errno == EINTR)
      return -1;

    perror("read @ read_message");
    exit(1);
  }

  return data_read;
}

/*========================================================================== */
