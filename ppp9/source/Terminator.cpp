#include <debug.h>
#include <Exceptions.h>
#include <cxxabi.h>

extern "C" std::type_info* __cxa_current_exception_type();

extern "C" void terminator() {
  std::type_info *t = __cxa_current_exception_type();
  iprintf("** exception %s reached top-level **\n", (t)?t->name():"unknown");
  die(0,0);
}

ExceptionTerminator::ExceptionTerminator()
{
  std::set_terminate(terminator);
}
