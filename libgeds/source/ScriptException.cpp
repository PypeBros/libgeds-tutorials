#include <sys/types.h>
#include <cstdarg>
#include <nds.h>
#include "interfaces.h"
iScriptException::iScriptException(const char* fmt, ...)
{
	va_list ap;
	va_start( ap,fmt);
	int n = vsniprintf( m_Message, MAXLEN, fmt, ap );
	va_end( ap );
	if(n==MAXLEN)
		m_Message[MAXLEN-1] = '\0';
}
