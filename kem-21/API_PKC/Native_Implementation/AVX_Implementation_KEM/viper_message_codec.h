#ifndef VIPER_MESSAGE_CODEC_H
#define VIPER_MESSAGE_CODEC_H

#include "viper.h"

void viper_msg_encode(vpoly out, const unsigned char m[VIPER_MSGBYTES]);
void viper_msg_decode(unsigned char m[VIPER_MSGBYTES], const vpoly in);
const char *viper_message_codec_name(void);

#endif
