#include <stdlib.h>
#include <signal.h>
#include <execinfo.h>
#include <Runtime/Runtime.h>
#include "SignalHandling.h"

bool doTerminate = false;

void signalHandler(int signal, struct __siginfo* info, void* ucontext)
{
    switch (signal)
    {
        case SIGQUIT:
        case SIGHUP:
        case SIGINT:
        case SIGPIPE:
        case SIGALRM:
        case SIGTERM:
        case SIGXCPU:
        case SIGXFSZ:
            doTerminate = true;
        break;

        case SIGILL:
        case SIGTRAP:
        case SIGABRT:
        case SIGEMT:
        case SIGFPE:
        case SIGBUS:
        case SIGSEGV:
        case SIGSYS:
            fprintf(stderr, "Received signal: %d\n", signal);
            RT::printBacktrace(STDERR_FILENO);
            exit(1);
        break;

        case SIGVTALRM:
        case SIGPROF:
        case SIGUSR1:
        case SIGUSR2:
        default:
        break;
    }
}

void installSignalHandlers()
{
    struct sigaction sigact;
    sigset_t sigset;
    int signals[] = { SIGHUP, SIGINT, SIGPIPE, SIGALRM, SIGTERM, SIGXCPU,
                      SIGXFSZ, SIGVTALRM, SIGUSR1, SIGUSR2, SIGQUIT, SIGABRT,
                      SIGTRAP, SIGEMT, SIGFPE, SIGBUS, SIGSEGV, SIGSYS,
                      SIGQUIT };

    sigfillset(&sigset);
    sigact.sa_mask = sigset;
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;
    sigact.__sigaction_u.__sa_sigaction = signalHandler;

    for (auto signal : signals) {
        if (sigaction(signal, &sigact, 0) == -1)
            RT::error(0x6ED49141, "%d %d\n", signal, errno);
    }
}
