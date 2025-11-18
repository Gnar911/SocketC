#include <stdio.h>

// IPC share information between processes, threads
// Between processes. Long time before.
// Share via filesystem (write to kernel file descriptor buffer)
// Share via Kernel/system calls
// Shared memory
//
// Between threads. POSIX standardized at 1995
// Share the same global variables
//
// A kernel-persistent IPC object remains in existence until the kernel reboots or
// until the object is explicitly deleted.
// Message queue
// Byte streams with no message boundaries
//
//
//
//
//
//
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
