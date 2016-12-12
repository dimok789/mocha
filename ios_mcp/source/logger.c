#include <stdarg.h>
#include <string.h>
#include "types.h"
#include "imports.h"
#include "socket.h"
#include "logger.h"

#ifdef DEBUG_LOGGER
static int log_socket = 0;

int log_init(unsigned int ipAddress)
{
	log_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (log_socket < 0)
		return log_socket;

	struct sockaddr_in connect_addr;
	memset(&connect_addr, 0, sizeof(connect_addr));
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = 4405;
	connect_addr.sin_addr.s_addr = ipAddress;

	if(connect(log_socket, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) < 0)
	{
	    closesocket(log_socket);
	    log_socket = -1;
	}

	return log_socket;
}

void log_deinit()
{
    if(log_socket >= 0)
    {
        closesocket(log_socket);
        log_socket = -1;
    }
}

static void log_print(const char *str, int len)
{
    int ret;
    while (len > 0) {
        int block = len < 1400 ? len : 1400; // take max 1400 bytes per UDP packet
        ret = send(log_socket, str, block, 0);
        if(ret < 0)
            break;

        len -= ret;
        str += ret;
    }
}

void log_printf(const char *format, ...)
{
    if(log_socket < 0) {
        return;
    }

    va_list args;
    va_start(args, format);

    char buffer[0x100];

    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    log_print(buffer, len);

    va_end(args);
}
#endif // DEBUG_LOGGER
