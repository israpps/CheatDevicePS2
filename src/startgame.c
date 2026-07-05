#include "startgame.h"
#include "graphics.h"
#include <kernel.h>
#include <sifrpc.h>
#include <stdio.h>
#include <sbv_patches.h>
#include <libcdvd.h>
#include <erl.h>
#include <libpad.h>
#include <tamtypes.h>

#include "menus.h"
#include "cheats.h"
#include "settings.h"
#include "util.h"
#include "objectpool.h"

#ifdef HDD
#include <fileXio_rpc.h>

extern int booting_from_hdd;
extern int HDD_USABLE;
extern char MountPoint[40];
int getMountInfo(char *path, char *mountString, char *mountPoint, char *newCWD);
#endif

typedef struct {
    u8  ident[16];  // struct definition for ELF object header
    u16 type;
    u16 machine;
    u32 version;
    u32 entry;
    u32 phoff;
    u32 shoff;
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
} elf_header_t;

typedef struct {
    u32 type;       // struct definition for ELF program section header
    u32 offset;
    void    *vaddr;
    u32 paddr;
    u32 filesz;
    u32 memsz;
    u32 flags;
    u32 align;
} elf_pheader_t;

#define GS_BGCOLOUR *((vu32*)0x120000e0)

extern u8   _bootstrap_elf_start[];
extern int _bootstrap_elf_size;

static int discPrompt()
{
    const char *items[] = {"Start Game", "Cancel"};
    char *activeGameTitle = cheatsGetActiveGameTitle();
    char promptText[128];

    if(activeGameTitle)
        snprintf(promptText, sizeof(promptText), "Please insert game disc for\n%s", activeGameTitle);
    else
        snprintf(promptText, sizeof(promptText), "Please insert game disc");

    return (displayPromptMenu(items, 2, promptText) == 0);
}

#define ELF_PT_LOAD 1

// Load boostrap into memory. Returns bootstrap entrypoint.
static void* loadBootstrap()
{
    elf_header_t *eh = (elf_header_t *)_bootstrap_elf_start;
    elf_pheader_t *eph = (elf_pheader_t *)(_bootstrap_elf_start + eh->phoff);

    int i;
    for (i = 0; i < eh->phnum; i++)
    {
        if (eph[i].type != ELF_PT_LOAD)
            continue;

        void *pdata = (void *)(_bootstrap_elf_start + eph[i].offset);
        memcpy(eph[i].vaddr, pdata, eph[i].filesz);

        if (eph[i].memsz > eph[i].filesz)
            memset(eph[i].vaddr + eph[i].filesz, 0, eph[i].memsz - eph[i].filesz);
    }

    return (void *)eh->entry;
}

#ifdef HDD
// Prepares booting an ELF from an "hdd0:partition:pfs:/path" style boot path:
// remounts pfs0: on the target partition and checks that the ELF exists, so
// failures show an error menu here instead of hanging in the bootstrap.
// On failure, pfs0: is restored to the boot partition and 0 is returned.
static int setupHddBootPath(const char *path)
{
    char msg[192];
    char pathCopy[100];
    char partition[64];
    char pfsPath[128];
    const char *file;
    int ret;

    if(!booting_from_hdd || !HDD_USABLE)
    {
        displayError("HDD boot paths can only be used when\nCheat Device was started from the HDD.");
        return 0;
    }

    file = strstr(path, ":pfs:");
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';

    if(!file || !getMountInfo(pathCopy, NULL, partition, NULL))
    {
        displayError("Invalid HDD boot path!\nExpected format:\nhdd0:partition:pfs:/path/to/boot.elf");
        return 0;
    }

    fileXioUmount("pfs0:");
    ret = fileXioMount("pfs0:", partition, FIO_MT_RDONLY);
    if(ret < 0)
    {
        snprintf(msg, sizeof(msg), "Error: failed to mount partition\n\"%s\" (%d)", partition, ret);
        fileXioMount("pfs0:", MountPoint, FIO_MT_RDWR);
        displayError(msg);
        return 0;
    }

    snprintf(pfsPath, sizeof(pfsPath), "pfs0:%s", file + 5);
    ret = fileXioOpen(pfsPath, O_RDONLY, 0);
    if(ret < 0)
    {
        snprintf(msg, sizeof(msg), "Error: couldn't open \"%s\"\non partition \"%s\" (%d)", file + 5, partition, ret);
        fileXioUmount("pfs0:");
        fileXioMount("pfs0:", MountPoint, FIO_MT_RDWR);
        displayError(msg);
        return 0;
    }
    fileXioClose(ret);

    return 1;
}
#endif

void startgameExecute(const char *path)
{
    static char boot2[100];

    if(strcmp(path, "==Disc==") == 0)
    {
        if(!discPrompt())
            return;

        graphicsDrawTextCentered(310, COLOR_YELLOW, "Starting game...");
        graphicsRender();

        // Wait for disc to be ready
        while(sceCdGetDiskType() == 1) {}
        sceCdDiskReady(0);
        
        int syscnfFile = open("cdrom0:\\SYSTEM.CNF;1", O_TEXT | O_RDONLY);
        if(!syscnfFile)
        {
            GS_BGCOLOUR = 0x1010B4; // red
            SleepThread();
        }
        
        char syscnfText[256];
        int syscnfLen = read(syscnfFile, syscnfText, 255);
        close(syscnfFile);
        if(!syscnfLen)
        {
            GS_BGCOLOUR = 0x1010B4;
            SleepThread();
        }
        
        syscnfText[syscnfLen] = '\0';

        int found = 0;
        char *line = strtok(syscnfText, "\n");
        while(line)
        {
            if(!found)
            {
                char *substr = strstr(line, "BOOT2");
                if(substr)
                {
                    substr += strlen("BOOT2");
                    while(*substr == ' ' || (*substr == '='))
                        substr++;
                    
                    strncpy(boot2, substr, 0x30);
                    found = 1;
                }
            }
            
            line = strtok(NULL, "\n");
        }
    }
    else
    {
        strncpy(boot2, path, sizeof(boot2) - 1);
        boot2[sizeof(boot2) - 1] = '\0';
    }

    // Databases and settings must be saved before an HDD boot path gets a
    // chance to remount pfs0: on another partition.
    cheatsSaveDatabase();
    settingsSave(NULL, 0);

    if(strncmp(boot2, "hdd0:", 5) == 0)
    {
#ifdef HDD
        if(!setupHddBootPath(boot2))
            return;
#else
        displayError("HDD boot paths are only supported by\nthe HDD build of Cheat Device.");
        return;
#endif
    }

    cheatsInstallCodesForEngine();
    killMenus();
    killCheats();
    killSettings();
    objectPoolKill();
    
    void *bootstrapEntrypoint = loadBootstrap();
    
    padPortClose(0, 0);
    
    fioExit();
    SifInitRpc(0);
    SifExitRpc();

    FlushCache(0); // data cache
    FlushCache(2); // instruction cache

    char *argv[2] = {boot2, "\0"};

    ExecPS2(bootstrapEntrypoint, 0, 2, argv);
};
