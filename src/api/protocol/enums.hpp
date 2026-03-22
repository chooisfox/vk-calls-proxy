#pragma once
#include <cstdint>

namespace API::PROTOCOL
{

enum class MsgType : uint8_t
{
	Connect	   = 0x01,
	ConnectOK  = 0x02,
	ConnectErr = 0x03,
	Data	   = 0x04,
	Close	   = 0x05,
	PauseRx	   = 0x06,
	ResumeRx   = 0x07,
	CallInvite = 0x08
};

} // namespace API::PROTOCOL
