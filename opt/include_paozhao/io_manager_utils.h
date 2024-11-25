#ifndef IO_MANAGER_UTILS_H
#define IO_MANAGER_UTILS_H

#include <array>
#include <map>
#include <vector>
#include <memory>

#define MAX_MESSAGE_SIZE 64

enum class IoChecksumType : int
{
	NO_CHECKSUM = 0,
	CRC16 = 1,
	SEEKING_CRC16,
	SEEKING_MODBUS,
};

namespace io_manager_utils
{
	class Message
	{
	public:
		Message();
		virtual ~Message() {}

		unsigned char *getMessage() { return reinterpret_cast<unsigned char *>(m_message.data()); }
		unsigned short getMessageLength() const { return m_iMessageLength; }

		virtual unsigned short headerSize() const = 0;

		virtual bool encode(const unsigned char *buffer, const unsigned short length) = 0;
		virtual bool decode(const unsigned char *buffer, const unsigned short length) = 0;
		virtual bool decode(const unsigned char *original, const unsigned char *buffer, const unsigned short length);
	protected:
		unsigned short m_iMessageLength;
		std::array<unsigned char, MAX_MESSAGE_SIZE> m_message;

		friend std::ostream& operator<<(std::ostream& os, const Message &data);
	};

	class RawMessage : public Message
	{
	public:
		RawMessage();
		virtual ~RawMessage() {}

		virtual unsigned short headerSize() const { return 0; }

		virtual bool encode(const unsigned char *buffer, const unsigned short length);
		virtual bool decode(const unsigned char *buffer, const unsigned short length);

	private:
		bool copy(const unsigned char *buffer, const unsigned short length);
	};

	class CRC16Message : public Message
	{
	public:
		CRC16Message();
		virtual ~CRC16Message() {};

		// only contain 2 bytes for CRC16 checksum, all others are message body itself
		virtual unsigned short headerSize() const { return 2; }

		virtual bool encode(const unsigned char *buffer, const unsigned short length);
		virtual bool decode(const unsigned char *buffer, const unsigned short length);

	protected:
		virtual bool matchChecksum(const unsigned char *buffer, const unsigned short length);
		virtual bool buildChecksum();

		unsigned short CRC16(const unsigned char *data, unsigned short length);
	};

	class SeekingCRC16Message : public CRC16Message
	{
	public:
		SeekingCRC16Message();
		virtual ~SeekingCRC16Message() {};

		// contains 4 header, 2 CRC16 checksum, and 1 tailer
		virtual unsigned short headerSize() const { return 7; }

		virtual bool encode(const unsigned char *buffer, const unsigned short length);
		virtual bool decode(const unsigned char *original, const unsigned char *buffer, const unsigned short length);
	private:
		virtual bool matchChecksum(const unsigned char *buffer, const unsigned short length);
		virtual bool buildChecksum();
	};

	class SeekingModbusMessage : public CRC16Message
	{
	public:
		SeekingModbusMessage();
		virtual ~SeekingModbusMessage() {};

		// contains 2 header (destination Id, and function code, and 2 CRC16 checksum
		virtual unsigned short headerSize() const { return 4; }

		virtual bool encode(const unsigned char unitId, const unsigned short address, const unsigned short quantity);
		virtual bool encode(const unsigned char unitId, const unsigned short address, const std::vector<short> &data);

		virtual bool decode(const unsigned char *original, const unsigned char *buffer, const unsigned short length);
		virtual bool decode(const unsigned char *buffer, const unsigned short length, std::vector<short> &data);
	};

	class MessageFactory
	{
	public:
		// Factory Method
		MessageFactory(const IoChecksumType type);
		virtual ~MessageFactory() {};

		std::shared_ptr<Message> getMessage() { return m_pMessage; }
		unsigned short msgHeaderSize() const { return m_iMsgHeaderSize; }
	private:
		unsigned short m_iMsgHeaderSize;
		std::shared_ptr<Message> m_pMessage;
	};

} // namespace io_manager_utils

#endif // IO_MANAGER_UTILS_H