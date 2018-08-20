#pragma once 

#include <stdarg.h>
#include <stdint.h>
#include <syslog.h>

namespace evwork
{

	enum LogLevel
	{
		Error = 3,
		Warn = 4,
		Notice = 5,
		Info = 6,
		Debug = 7
	};

	class ILogReport
	{
	public:
		virtual ~ILogReport() {}

		virtual void log(int iLevel, const char* szFormat, ...) = 0;
	};

	class ILogReportAware
	{
	protected:
		ILogReport* m_pLogReport;
	public:
		ILogReportAware() : m_pLogReport(NULL) {}
		virtual ~ILogReportAware() {}

		void setLogReport(ILogReport* p) { m_pLogReport = p; }
		ILogReport* getLogReport() { return m_pLogReport; }
	};

	class CSyslogReport
		: public ILogReport
	{
	public:

		CSyslogReport()
		{
			openlog(NULL, LOG_PID, LOG_USER);
		}
		virtual ~CSyslogReport()
		{
			closelog();
		}

		virtual void log(int iLevel, const char* szFormat, ...)
		{
			va_list va;
			va_start(va, szFormat);

			vsyslog(iLevel, szFormat, va);

			va_end(va);
		}
	};
}
