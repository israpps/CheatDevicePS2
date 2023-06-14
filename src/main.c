#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <debug.h>
#include <libpad.h>

#include "graphics.h"
#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "dbgprintf.h"
#include "util.h"

int main(int argc, char *argv[])
{
    DPRINTF_INIT();
    printf("Cheat Device. By wesley castro. Maintained by El_isra\n Compilation " __DATE__ " " __TIME__ "\n");
    DPRINTF("Cheat Device. By wesley castro. Maintained by El_isra\n Compilation " __DATE__ " " __TIME__ "\n");
    initGraphics();
    loadModules();
    initSettings();
    initMenus();
    
    char *readOnlyPath = settingsGetReadOnlyDatabasePath();
    if(readOnlyPath && !cheatsOpenDatabase(readOnlyPath, 1))
    {
        char error[255];
        sprintf(error, "Error loading read-only cheat database \"%s\"!", readOnlyPath);
        displayError(error);
    }

    char *readWritePath = settingsGetReadWriteDatabasePath();
    if(readWritePath && !cheatsOpenDatabase(readWritePath, 0))
    {
        char error[255];
        sprintf(error, "Error loading read/write cheat database \"%s\"!", readWritePath);
        displayError(error);
    }
    
    cheatsLoadGameMenu();
    cheatsLoadHistory();

    /* Main Loop */
    while(1)
    {
        handlePad();
        graphicsRender();
    }
    
    killCheats();
    SleepThread();
    return 0;
}
