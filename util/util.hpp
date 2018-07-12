#pragma once

namespace fnc
{

#define STRING_JOIN(arg1, arg2) STRING_JOIN2(arg1, arg2)
#define STRING_JOIN2(arg1, arg2) arg1 ## arg2

#define STRINGIZE(x) STRINGIZE_SIMPLE(x)
#define STRINGIZE_SIMPLE(x) #x


} // end namespace fnc
