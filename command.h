#ifndef _COMMAND_H_
#define _COMMAND_H_

#define PC_RC_RESET (-1)
#define PC_RC_STOP (-2)

char command_process(unsigned char* cmd_str);
const char* command_tab_complete(const char* cmd, unsigned short cmd_len, unsigned short* match_count);

#endif // _COMMAND_H_
