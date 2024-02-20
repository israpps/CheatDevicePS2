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

#ifdef HDD
int getMountInfo(char *path, char *mountString, char *mountPoint, char *newCWD);
#include <fileXio_rpc.h>
#include <assert.h>
int HDD_USABLE = 0;
char MountPoint[40];
char pfspath[64];
#endif

int booting_from_hdd = 0;
char error[255];
int main(int argc, char *argv[])
{
    DPRINTF_INIT();
    int ret = 0;
    printf("Cheat Device. By wesley castro. Maintained by El_isra\n Compilation " __DATE__ " " __TIME__ "\n");
    DPRINTF("Cheat Device. By wesley castro. Maintained by El_isra\n Compilation " __DATE__ " " __TIME__ "\n");
    initGraphics();
#ifndef NO_DPRINTF
    for (ret=0;ret<argc;ret++) {DPRINTF("argv[%d]: '%s'\n", ret, argv[ret]);}
    ret = 0;
#endif
#ifdef HDD
    DPRINTF("Checking if booting from HDD\n");
    if (argc > 0) booting_from_hdd = (strstr(argv[0], "hdd0:")!=NULL)&&(strstr(argv[0], ":pfs:")!=NULL);
    DPRINTF("Booting from hdd:%d\n", booting_from_hdd);
    char* BUF = NULL;
    BUF = strdup(argv[0]); //use strdup, otherwise, path will become `hdd0:`
    if (BUF==NULL) {
        DPRINTF("Could not strdup()\n");
    }
    if (getMountInfo(BUF, NULL, MountPoint, pfspath)) {
        
    DPRINTF("MountPoint '%s'\npfspath '%s'\n", MountPoint, pfspath);
    char *pos = strrchr(pfspath, '/');
    if (pos != NULL) {
        pos++;
        *pos = '\0';
        char* B = strrchr(pfspath, '/');
        if (B!=NULL) { //the path includes folders inside the PFS filesystem?
            pfspath[(B-pfspath+1)]=0; //null terminate after the last '/', where the elf filename should begin?
            DPRINTF("boot path is not root of pfs\n");
        } else {
            strcpy(pfspath, "pfs:");
        }
        DPRINTF("chdir: '%s'\n", pfspath);
        chdir(pfspath);
    } else displayError("Error processing HDD boot path (2)");
    } else displayError("Error processing HDD boot path (1)");
#endif
    
    ret = loadModules(booting_from_hdd);
    if (ret != 0) displayError(error);
#ifdef HDD
    if (ret == 0) {
        int mtret=0;
        if ((mtret=fileXioMount("pfs0:", MountPoint, FIO_MT_RDWR)) < 0) {
            sprintf(error, "Error: failed to mount partition \"%s\"!\nerr:%d (0x%x)", MountPoint, mtret, mtret);
            DPRINTF(error);
            displayError(error);
        } else {
            DPRINTF("Successful HDD boot. mounted %s as pfs0\n", MountPoint);
        }
    }
#endif
    initSettings();
    initMenus();
    
    char *readOnlyPath = settingsGetReadOnlyDatabasePath();
    if(readOnlyPath && !cheatsOpenDatabase(readOnlyPath, 1))
    {
        sprintf(error, "Error loading read-only cheat database \"%s\"!", readOnlyPath);
        displayError(error);
    }

    char *readWritePath = settingsGetReadWriteDatabasePath();
    if(readWritePath && !cheatsOpenDatabase(readWritePath, 0))
    {
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

#ifdef HDD
//By fjtrujy
char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

/**
 * @brief  method returns true if it can extract needed info from path, otherwise false.
 * In case of true, it also updates mountString, mountPoint and newCWD parameters
 * It splits path by ":", and requires a minimum of 3 elements
 * Example: if path = hdd0:__common:pfs:/retroarch/ then
 * mountString = "pfs:"
 * mountPoint = "hdd0:__common"
 * newCWD = pfs:/retroarch/
 * return true
*/
int getMountInfo(char *path, char *mountString, char *mountPoint, char *newCWD)
{
    int expected_items = 4;
    int i = 0;
    char *items[expected_items];
    char** tokens = str_split(path, ':');

    if (!tokens)
        return 0;
    
    for (i = 0; *(tokens + i); i++) {
        if (i < expected_items) {
            items[i] = *(tokens + i);
        } else {
            free(*(tokens + i));    
        }
    }

    if (i < 3 )
        return 0;
    
    if (mountPoint != NULL)
        sprintf(mountPoint, "%s:%s", items[0], items[1]);

    if (mountString != NULL)
        sprintf(mountString, "%s:", items[2]);

    if (newCWD != NULL)
        sprintf(newCWD, "%s:%s", items[2], i > 3 ? items[3] : "");
    
    free(items[0]);
    free(items[1]);
    free(items[2]);

    if (i > 3)
        free(items[3]);

    return 1;
}
#endif
