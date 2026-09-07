#ifdef QIMING_DIAGNOSTIC

#include "xparameters.h"
#include "xuartps_hw.h"

namespace {

void early_uart_marker()
{
    static const char marker[] = "\r\n[EARLY] application runtime entered\r\n";
    for (unsigned int i = 0; i < sizeof(marker) - 1; ++i) {
        XUartPs_SendByte(STDOUT_BASEADDRESS, marker[i]);
    }
}

typedef void (*PreinitFunction)();
__attribute__((used, section(".preinit_array"))) PreinitFunction early_marker = early_uart_marker;

} // namespace

#endif
