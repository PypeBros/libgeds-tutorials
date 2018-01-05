#include "nds.h"
#include "debug.h"
#include <cxxabi.h>
#include <sys/types.h>

/** these are heavy guys from the lib i want to strip out. **/
extern "C" char* _dtoa_r(_reent*, double, int, int, int*, int*, char**) {
  die(__FILE__, __LINE__);
  return (char*)"X.X";
} /** saves 397 - 388KB **/


/** slim fast :) **/
extern "C" char* __cxa_demangle(const char* mangled_name,
		       char* output_buffer, size_t* length,
		       int* status) {
  if (status) *status = -2;
  return 0;
} /* saves 403 - 388 KB */

extern "C" void _jp2uc(void) {
}

extern "C" int mbtowc(wchar_t*, const char*, size_t) {
  return 0;
}

extern "C" int wctomb(char*, wchar_t) {
  return 0;
}
