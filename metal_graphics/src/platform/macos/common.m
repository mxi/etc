#include <stdarg.h>

#import <Foundation/Foundation.h>

#include "../common.h"

void log_message(int level, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    NSLogv([NSString stringWithCString:format encoding:NSUTF8StringEncoding], ap);
    va_end(ap);
}