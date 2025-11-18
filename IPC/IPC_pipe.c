#include <stdio.h>

// IPC share information between processes, threads
// Between processes. Long time before.
// Share via filesystem (write to file)
// Share via Kernel/system calls
// Shared memory
// Between threads. POSIX standardized at 1995
// Share the same global variables
//
//
// PIPE
// One FD for writing
// One FD for reading
// A normal file descriptor refers to a file on disk.
// A pipe file descriptor refers to an in-kernel memory buffer.
// There is no file on the filesystem.
// Parent process:
// int fd[2];
// pipe(fd);
// This attack the file into the file descriptor table of the process
// fd_table[0] → stdin
// fd_table[1] → stdout
// fd_table[2] → stderr
// fd_table[3] → your pipe read end
// fd_table[4] → your pipe write end
//
// write(fd[1], "hello", 5);
// read(fd[0], buf, 5);
//
//
// FIFO
// The benefit in the extra calls required for the FIFO is that a FIFO has a name in the file-
// system allowing one process to create a FIFO and another unrelated process to open the
// FIFO. This is not possible with a pipe.
// FIFO1, FIFO2
//
// If both the parent and the child are opening a FIFO for reading when no process
// has the FIFO open for writing. -> Deadlock
//
// Pipes and FIFOs, have used the stream I/O model to exchange bytes
// 1. Read one line at a time (No need message structure, only data each line)
// 2. Use the 2-character sequence of a carriage return followed by a liiefeed (CR/LF)
// 3. Explicit length: each record is preceded by its length. (Use message structure)
// 4. One record per connection HTTP 1.0
//
// Any data remaining in a pipe or FIFO when the last close of the pipe or FIFO takes place, is discarded.
// The data and length has the limitation under the system chracteristic
//
//
//
//
//
//
//
int main()
{
    printf("Hello World!\n");
    return 0;
}
