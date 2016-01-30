#ifndef _BASETYPE_H_
#define _BASETYPE_H_

#include"utils.h"

enum PackType
{
	PACK_SERVER_FILE_BLOCK,
	PACK_SERVER_FILE_INFO,
	PACK_CLIENT_REQUEST_DOWNFILE,
	PACK_CLIENT_BEGIN_DOWNFILE,
	PACK_CLIENT_CLOSED_DOWNFILE,

	PACK_SEND_MSG,
	PACK_RESPONSE_SEND_MSG,
	PACK_PLAYER_AMOUNT_INFO,
	PACK_PLAYER_UPDATE_DATA,
	PACK_RESPONSE_PLAYER_AMOUNT_INFO,
	PACK_RESPONSE_PLAYER_UPDATE_DATA,
	PACK_REQUEST_GAME_PARLOR,
	PACK_GAME_SIGNUP,
	PACK_GAME_PLAY,
	PACK_SOCKET_OFFLINE,
	PACK_SOCKET_CONNECTED,
	PACK_SOCKET_ACCEPTED,
	PACK_HEARTBEAT,
	PACK_RESPONSE_HEARTBEAT,
	PACK_PLAYER_REQUESTGAME,
};

struct PackHeader
{
	uint16_t type;
	uint16_t len;
};

#endif