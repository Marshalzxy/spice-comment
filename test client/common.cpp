#include "stdafx.h"
#include "type.h"
#include "command.h"
#include <winsock2.h>
void op_common_correct_endian(struct op_common	*cmd,int send){
	if(send){
		cmd->version = htons(cmd->version);
		cmd->code = htons(cmd->code);
		cmd->status = htonl(cmd->status);
		cmd->magic = htonl(cmd->magic);
	}else{
		cmd->version = ntohs(cmd->version);
		cmd->code = ntohs(cmd->code);
		cmd->status = ntohl(cmd->status);
		cmd->magic = ntohl(cmd->magic);
	}

}