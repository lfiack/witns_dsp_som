// #include <sys/stat.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <stdio.h>
// #include <signal.h>
// #include <time.h>
// #include <sys/time.h>
// #include <sys/times.h>

#include "ff.h"
#include <fcntl.h>
#include <stdarg.h>

#include "main.h"
#include "usart.h"

#define MAX_OPEN_FILES 16

static FIL file_pool[MAX_OPEN_FILES];
static uint8_t file_used[MAX_OPEN_FILES] = {0};

static int alloc_file_slot(void)
{
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_used[i]) {
            file_used[i] = 1;
            return i;
        }
    }
    return -1; // plus de slots disponibles
}

static void free_file_slot(int fd)
{
    if (fd >= 0 && fd < MAX_OPEN_FILES)
        file_used[fd] = 0;
}

/* ------------------------------------------------------------------------- */

int _open(char *path, int flags, ...)
{
    int fd = alloc_file_slot();
    if (fd < 0)
        return -1;

    // flags = 65536 for "rb"
    // flags = 67073 for "wb"

    BYTE fatfs_flags = FA_READ;
    if (flags & O_RDONLY)
        fatfs_flags |= FA_READ;
    if (flags & O_WRONLY)
        fatfs_flags |= FA_WRITE;
    if (flags & O_RDWR)
        fatfs_flags |= FA_READ | FA_WRITE;
    if (flags & O_CREAT)
        fatfs_flags |= FA_CREATE_ALWAYS;

    FRESULT res = f_open(&file_pool[fd], path, fatfs_flags);
    if (res != FR_OK) {
        free_file_slot(fd);
        return -1;
    }

    return fd + MAX_OPEN_FILES; // ← on retourne simplement l’indice
}

int _close(int fd)
{
    fd -= MAX_OPEN_FILES;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_used[fd])
        return -1;

    f_close(&file_pool[fd]);
    free_file_slot(fd);
    return 0;
}

int _write(int fd, char *ptr, int len)
{
    if (fd < MAX_OPEN_FILES)
    {
        if (HAL_UART_Transmit(&huart4, (uint8_t*)ptr, len, HAL_MAX_DELAY) == HAL_OK)
        {
            return len;
        }
        else {
            return -1;
        }
    }

    fd -= MAX_OPEN_FILES;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_used[fd])
        return -1;

    UINT written;
    FRESULT res = f_write(&file_pool[fd], ptr, len, &written);
    return (res == FR_OK) ? written : -1;
}

int _read(int fd, char *ptr, int len)
{
    fd -= MAX_OPEN_FILES;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_used[fd])
        return -1;

    UINT read;
    FRESULT res = f_read(&file_pool[fd], ptr, len, &read);
    return (res == FR_OK) ? read : -1;
}

int _lseek(int fd, int offset, int whence)
{
    fd -= MAX_OPEN_FILES;

    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_used[fd])
        return -1;

    FSIZE_t pos = 0;
    if (whence == 0) pos = offset; // SEEK_SET
    else if (whence == 1) pos = f_tell(&file_pool[fd]) + offset; // SEEK_CUR
    else if (whence == 2) pos = f_size(&file_pool[fd]) + offset; // SEEK_END

    FRESULT res = f_lseek(&file_pool[fd], pos);
    return (res == FR_OK) ? (int)pos : -1;
}