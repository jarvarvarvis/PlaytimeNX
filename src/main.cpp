#include "app.hpp"
#include <switch.h>

extern "C" {
#ifndef NDEBUG
#include <unistd.h> // for close()
static int nxlink_socket;
#endif // NDEBUG
#define THROW_IF(func) if (auto r = func; R_FAILED(r)) fatalThrow(r);

void userAppInit(void) {
    THROW_IF(appletLockExit());
    THROW_IF(romfsInit());
    THROW_IF(plInitialize(PlServiceType_User));
    THROW_IF(nsInitialize());
    THROW_IF(pdmqryInitialize());
    THROW_IF(accountInitialize(AccountServiceType_Administrator));
#ifndef NDEBUG
    THROW_IF(socketInitializeDefault());
    nxlink_socket = nxlinkStdio();
#endif // NDEBUG
}

void userAppExit(void) {
#ifndef NDEBUG
    close(nxlink_socket);
    socketExit();
#endif // NDEBUG
    accountExit();
    pdmqryExit();
    plExit();
    nsExit();
    romfsExit();
    appletUnlockExit();
}
} // extern "C"

int main(int argc, char** argv) {
    tj::App app{};
    app.RequestAccountUid();
    app.SpawnScanThread();
    app.Loop();
    return 0;
}
