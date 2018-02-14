#ifndef __NOCOPY_H
#define __NOCOPY_H

#define NOCOPY(__x) __x& operator=(const __x &that); __x(const __x &that);

#endif
