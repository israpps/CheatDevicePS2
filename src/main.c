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
    char *readWritePath = NULL;
    char *readOnlyPath = NULL;
    int x;

    DPRINTF_INIT();
    printf("Cheat Device. By wesley castro. Maintained by El_isra\n Compilation " __DATE__ " " __TIME__ "\n");
    DPRINTF("Cheat Device. By wesley castro. Maintained by El_isra\n Compilation " __DATE__ " " __TIME__ "\n");
    initGraphics();
    loadModules();
    initSettings();
    initMenus();
    for (x=0; x<argv;x++)
    {
        if (!strncmp("-ord=", argv[x], 5))
        {
            DPRINTF("Overriding readonly database path with '%s' from argv[%d]\n", &argv[x][5], x);
            settingsSetReadOnlyDatabasePath(&argv[x][5]);
        }
        if (!strncmp("-owd=", argv[x], 5))
        {
            DPRINTF("Overriding read/write database path with '%s' from argv[%d]\n", &argv[x][5], x);
            settingsSetReadWriteDatabasePath(&argv[x][5]);
        }
    }
    readOnlyPath = settingsGetReadOnlyDatabasePath();
    if(readOnlyPath && !cheatsOpenDatabase(readOnlyPath, 1))
    {
        char error[255];
        sprintf(error, "Error loading read-only cheat database \"%s\"!", readOnlyPath);
        displayError(error);
    }

    readWritePath = settingsGetReadWriteDatabasePath();
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
