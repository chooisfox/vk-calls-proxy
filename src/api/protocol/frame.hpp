#pragma once
#include "enums.hpp"

#include <QByteArray>

namespace API::PROTOCOL
{

struct Frame
{
	uint32_t   connId;
	MsgType	   type;
	QByteArray payload;
};

} // namespace API::PROTOCOL
