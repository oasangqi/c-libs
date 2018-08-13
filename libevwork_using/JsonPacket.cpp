#include "JsonPacket.h"

#include "EVWork.h"

using namespace evwork;

#define XOR_KEY		16

Jpacket::Jpacket()
{
}
Jpacket::~Jpacket()
{
}

std::string& Jpacket::tostring()
{
	return str;
}

Json::Value& Jpacket::tojson()
{
	return val;
}

void Jpacket::end()
{
#ifdef __DEBUG__
	Json::FastWriter writer;
	std::string strJsonText = writer.write(val);

	LOG(Debug, "[Jpacket::%s] sendDataStyled:[%s]", __FUNCTION__, strJsonText.c_str());
#endif

	std::string out = val.toStyledString();

	xorfunc(out);
	header.length = out.length();

	str.clear();
	str.append((const char *)&header, sizeof(struct Header));
	str.append(out);
}

int Jpacket::parse(std::string& str)
{
	xorfunc(str);

	if (!reader.parse(str, val))
	{
		LOG(Error, "[Jpacket::%s] recv data format error!", __FUNCTION__);
		return -1;
	}

#ifdef __DEBUG__
	Json::FastWriter writer;
	std::string strJsonText = writer.write(val);

	LOG(Debug, "[Jpacket::%s] recvData:[%s]", __FUNCTION__, strJsonText.c_str());
#endif

	return 0;
}

void Jpacket::xorfunc(std::string& str)
{
	const int KEY = XOR_KEY; // 0x10
	int strLen = str.length();
	char *cString = (char*)str.c_str();

	for (int i = 0; i < strLen; i++)
	{
		*(cString + i) = (*(cString + i) ^ KEY);
	}
}

Jpacket2::Jpacket2()
: val(rapidjson::kObjectType)
{
}
Jpacket2::~Jpacket2()
{
}

rapidjson::Document::AllocatorType& Jpacket2::getAllocator()
{
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
	return allocator;
}

std::string& Jpacket2::tostring()
{
	return str;
}

void Jpacket2::end()
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	val.Accept(writer);

	std::string out = buffer.GetString();

#ifdef __DEBUG__
	LOG(Debug, "[Jpacket2::%s] sendDataStyled:[%s]", __FUNCTION__, out.c_str());
#endif

	xorfunc(out);
	header.length = out.length();

	str.clear();
	str.append((const char *)&header, sizeof(struct Header));
	str.append(out);
}

int Jpacket2::parse(std::string& str)
{
	xorfunc(str);

	rapidjson::Document doc;
	doc.Parse<0>(str.c_str());

	if (doc.HasParseError())
	{
		LOG(Error, "[Jpacket2::%s] recv data format error!", __FUNCTION__);
		return -1;
	}

#ifdef __DEBUG__
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	std::string strJsonText = doc.GetString();

	LOG(Debug, "[Jpacket2::%s] recvData:[%s]", __FUNCTION__, strJsonText.c_str());
#endif

	return 0;
}

void Jpacket2::xorfunc(std::string& str)
{
	const int KEY = XOR_KEY;
	int strLen = str.length();
	char *cString = (char*)str.c_str();

	for (int i = 0; i < strLen; i++)
	{
		*(cString + i) = (*(cString + i) ^ KEY);
	}
}
