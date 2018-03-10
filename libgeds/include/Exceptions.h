#ifndef __LIBGEDS_EXCEPTIONS
#define __LIBGEDS_EXCEPTIONS
class iScriptException {
public:
  iScriptException( const char* fmt, ... );
  const char* what() const
  { return m_Message; }
protected:
  enum { MAXLEN=256 };
  char m_Message[ MAXLEN ];
};

/** when instanciated, this prevents exceptions from reaching 'top level'
 */
class ExceptionTerminator {
  static bool installed;
 public:
  ExceptionTerminator();
};

#ifdef IS_FREED_MEMORY
#define CHECK_POINTER(p, msg) if (IS_FREED_MEMORY(p)) { throw msg; }
#else
#define CHECK_POINTER(p, msg)
#endif
#endif
