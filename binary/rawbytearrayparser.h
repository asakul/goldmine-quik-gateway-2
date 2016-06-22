/*
 * rawbytearrayparser.h
 */

#ifndef RAWBYTEARRAYPARSER_H_
#define RAWBYTEARRAYPARSER_H_

#include <cstdint>
#include <stdexcept>

class RawByteArrayParser
{
public:
	class StreamEndException : public std::runtime_error
	{
	public:
		StreamEndException(const std::string& message = std::string()) :
			std::runtime_error(message) {}
	};

	enum Endianness
	{
		BigEndian,
		LittleEndian
	};

	RawByteArrayParser(const void* buffer, int bufsize, Endianness endianness = LittleEndian);
	virtual ~RawByteArrayParser();

	uint8_t readByte();
	uint16_t readWord();
	uint32_t readDword();
	void readBytes(void* buffer, size_t count);
	size_t readAll(void* buffer, size_t buffer_size);

	void skip(int bytes);

	bool atEnd();

private:
	RawByteArrayParser(const RawByteArrayParser& parser) : m_endianness(LittleEndian) {}
	RawByteArrayParser& operator=(const RawByteArrayParser& other) { return *this; }
	const uint8_t* m_ptr;
	size_t m_left;
	Endianness m_endianness;
};

#endif /* RAWBYTEARRAYPARSER_H_ */
