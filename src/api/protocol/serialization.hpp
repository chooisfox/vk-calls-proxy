#pragma once
#include "frame.hpp"

#include <QtEndian>
#include <optional>

namespace API::PROTOCOL
{

[[nodiscard]] inline std::optional<Frame> decode(const QByteArray& data) noexcept
{
	if (data.size() < 5)
		return std::nullopt;

	Frame frame;
	frame.connId  = qFromBigEndian<uint32_t>(data.constData());
	frame.type	  = static_cast<MsgType>(data[4]);
	frame.payload = data.mid(5);

	return frame;
}

[[nodiscard]] inline QByteArray encode(const Frame& frame)
{
	QByteArray data(5 + frame.payload.size(), Qt::Uninitialized);
	qToBigEndian(frame.connId, data.data());
	data[4] = static_cast<char>(frame.type);

	if (!frame.payload.isEmpty())
	{
		std::memcpy(data.data() + 5, frame.payload.constData(), frame.payload.size());
	}

	return data;
}

} // namespace API::PROTOCOL
