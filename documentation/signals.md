# OBOS Signals
## Definition
A callback (much like an interrupt) called by the kernel on a certain condition (ex: page fault).
## Signal values.
| Signal  | Value  | Reason for signal                                                                                                                                                                               | Default action    |
|---------|--------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|
| SIGPF   | 0x0000 | A page fault occurred that the kernel couldn't resolve (not related to demand paging or any VMM feature).                                                                                       | Kill the process. |
| SIGPM   | 0x0001 | Permission error. This occurs when the program tries to modify something other than memory that it is not allowed to modify.                                                                    | Kill the process. |
| SIGOF   | 0x0002 | An integer overflow occurred                                                                                                                                                                    | Kill the process. |
| SIGME   | 0x0003 | A math error occurred. This can occur on division by zero or any operation that involves math (like anything to do with floating point). This should not occur on integer overflow (see SIGOF). | Kill the process. |
| SIGDG   | 0x0004 | A debug exception occurred. This is architecture-specific.                                                                                                                                      | Kill the process. |
| SIGTIME | 0x0005 | A timer set by the thread finished.                                                                                                                                                             | Ignore            |
| SIGTERM | 0x0006 | A program made a request to terminate the program. This is provided so that the program can properly free any resources opened.                                                                 | Kill the process. |
| SIGINT  | 0x0007 | A request to interrupt the process was made.                                                                                                                                                    | Kill the process. |
| SIGUDOC | 0x0008 | An undefined opcode exception occurred.                                                                                                                                                         | Kill the process. |
| SIGUEXC | 0x0009 | An exception with no signal happened.                                                                                                                                                           | Kill the process. |