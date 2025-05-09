
#ifndef CLIENT_H
#define CLIENT_H


// concerning syncing of clients to the game state


/**
 * Parse messages received from a client,
 * and kick off any commands as appopriate
 */
void server_parse_msg(const char* cmd);

#endif