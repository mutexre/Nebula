#include <stdlib.h>
#include <signal.h>
#include <boost/format.hpp>
#include <Runtime/Runtime.h>
#include "SignalHandling.h"

bool didReceivedSIGTERM = false;

void signalFunc(int signal)
{
    switch (signal)
    {
        case SIGHUP:
        case SIGINT:
        case SIGPIPE:
        case SIGALRM:
        case SIGTERM:
        case SIGXCPU:
        case SIGXFSZ:
        case SIGVTALRM:
        case SIGPROF:
        case SIGUSR1:
        case SIGUSR2:
            didReceivedSIGTERM = true;
        break;

        case SIGQUIT:
        case SIGILL:
        case SIGTRAP:
        case SIGABRT:
        case SIGEMT:
        case SIGFPE:
        case SIGBUS:
        case SIGSEGV:
        case SIGSYS:
            RT::error(0xB1ACDE28, "Process did receive signal %u.\n", signal);
            exit(-1);
        break;

        default:
        break;
    }
}

void installSignalHandlers()
{
    if (signal(SIGHUP, signalFunc) == SIG_ERR)
        RT::error(0x7EE57579, "%d\n", errno);

    if (signal(SIGINT, signalFunc) == SIG_ERR)
        RT::error(0xF69A0723, "%d\n", errno);

    if (signal(SIGPIPE, signalFunc) == SIG_ERR)
        RT::error(0x6C974817, "%d\n", errno);

    if (signal(SIGALRM, signalFunc) == SIG_ERR)
        RT::error(0xF9CE3F93, "%d\n", errno);

    if (signal(SIGTERM, signalFunc) == SIG_ERR)
        RT::error(0x9D3F8FF5, "%d\n", errno);

    if (signal(SIGXCPU, signalFunc) == SIG_ERR)
        RT::error(0x29DD994A, "%d\n", errno);

    if (signal(SIGXFSZ, signalFunc) == SIG_ERR)
        RT::error(0x4EE59BFA, "%d\n", errno);

    if (signal(SIGVTALRM, signalFunc) == SIG_ERR)
        RT::error(0x6E3AD2E7, "%d\n", errno);

    if (signal(SIGUSR1, signalFunc) == SIG_ERR)
        RT::error(0x9FFDD262, "%d\n", errno);

    if (signal(SIGUSR2, signalFunc) == SIG_ERR)
        RT::error(0x8FCEAA85, "%d\n", errno);

    if (signal(SIGQUIT, signalFunc) == SIG_ERR)
        RT::error(0x3814FF69, "%d\n", errno);

    if (signal(SIGABRT, signalFunc) == SIG_ERR)
        RT::error(0xD4889919, "%d\n", errno);

    if (signal(SIGTRAP, signalFunc) == SIG_ERR)
        RT::error(0xEB9B877B, "%d\n", errno);

    if (signal(SIGEMT, signalFunc) == SIG_ERR)
        RT::error(0xC0EC1CF4, "%d\n", errno);

    if (signal(SIGFPE, signalFunc) == SIG_ERR)
        RT::error(0xFE67A7A4, "%d\n", errno);

    if (signal(SIGBUS, signalFunc) == SIG_ERR)
        RT::error(0x9460920E, "%d\n", errno);

    if (signal(SIGSEGV, signalFunc) == SIG_ERR)
        RT::error(0x17FC46B9, "%d\n", errno);

    if (signal(SIGSYS, signalFunc) == SIG_ERR)
        RT::error(0xD9782E5E, "%d\n", errno);
}
