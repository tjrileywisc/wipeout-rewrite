#pragma once

// Single-byte type discriminator prepended to every protobuf packet.
// Plain-text messages ("connect", "connected", etc.) do not carry a type byte.
#define MSG_TYPE_SERVER_INFO  0x01
#define MSG_TYPE_CLIENT_LIST  0x02
