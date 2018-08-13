#pragma once 

#include <stdint.h>

#include <json/json.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#undef HEADER_SIZE
#define HEADER_SIZE		4

namespace evwork
{

	struct Header
	{
		unsigned int	length;

		static uint32_t peekLen(const void * d)
		{
			return *((uint32_t*)d);
		}
	};

	class Jpacket
	{
	public:
		Jpacket();
		virtual ~Jpacket();

		std::string& tostring();
		Json::Value& tojson();

		void end();
		int parse(std::string& str);

		static void xorfunc(std::string& str);

	private:
		struct Header           header;

	public:
		std::string             str;
		Json::Value             val;
		Json::Reader            reader;
		int                     len;
	};

	class Jpacket2
	{
	public:
		Jpacket2();
		virtual ~Jpacket2();

		rapidjson::Document::AllocatorType& getAllocator();

		std::string& tostring();

		void end();

		int parse(std::string& str);

		static void xorfunc(std::string& str);

	private:
		struct Header           header;

	public:
		std::string             str;

		rapidjson::Document		doc;
		rapidjson::Value		val;
	};

}
