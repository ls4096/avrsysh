#ifndef _COMMAND_H_
#define _COMMAND_H_

#define PC_RC_RESET (-1)
#define PC_RC_STOP (-2)

char process_command(unsigned char* cmd_str);

#endif // _COMMAND_H_
