#include "TermPrint.h"

#ifdef SOCKET_SERVER_PORT
NetClient* CTermPrint::socketServerClient_ = nullptr;   // out-of-class definition, no "static" here
#endif
