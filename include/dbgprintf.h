#ifndef DPRINTF_MACRO
#define DPRINTF_MACRO

#ifdef COMMON_PRINTF
    #define DPRINTF(x...) printf(x)
    #define DPUTS(x) puts(x)
#endif

#ifdef EE_SIO
    #include <sio.h>
    #define DPRINTF(x...) sio_printf(x)
    #define DPUTS(x) sio_puts(x)
    #define DPRINTF_INIT() sio_init(38400, 0, 0, 0, 0)
#endif

#ifndef DPRINTF
    #define DPRINTF(x...)
    #define DPUTS(x)
    #define NO_DPRINTF
#endif

#ifndef DPUTS
    #define DPUTS(x)
    #define NO_DPUTS
#endif

#ifndef DPRINTF_INIT
    #define DPRINTF_INIT()
    #define NO_DPRINTF_INIT
#endif

#endif
