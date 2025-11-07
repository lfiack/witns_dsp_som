#include "sd.h"

#include <stdio.h>
#include "ff.h"
#include "parameter.h"
#include <string.h>

FATFS fs;

static uint8_t sd_parse_line(const char * line, h_param_t * h_param);

uint8_t sd_init(void)
{
    return f_mount(&fs, "", 1);  // "" = "0:", 1 = mount now
}

uint8_t sd_ls(const char *path)
{
	FRESULT res;
	DIR dir;
	FILINFO fno;

	res = f_opendir(&dir, path);  // Ouvre le répertoire
    if (res == FR_OK) 
    {
        while (1) 
        {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
            {
                break; // Fin
            }
            printf("%s%s\r\n", fno.fname, (fno.fattrib & AM_DIR) ? "/" : "");
        }
        return f_closedir(&dir);
    } 

    return res;

}

uint8_t sd_cat(const char *filename)
{
	FIL file;
	FRESULT res;
	char buffer[128];
	UINT bytes_read;

	res = f_open(&file, filename, FA_READ);
	if (res == FR_OK) {
		printf("Reading %s:\r\n", filename);
		do {
			res = f_read(&file, buffer, sizeof(buffer)-1, &bytes_read);
			if (res != FR_OK) break;
			buffer[bytes_read] = '\0';
			printf("%s", buffer);
		} while (bytes_read == sizeof(buffer)-1);
		printf("\r\n-- End of file --\r\n");
		return f_close(&file);
	} 

	return res;
}

uint8_t sd_save_param(const char *filename, h_param_t * h_param)
{
	FIL file;
	FRESULT res;
    char buffer[128];

    printf("Saving params\r\n");

    // TODO suppose FA_CREATE_ALWAYS means it overwrite ; TODO test with FA_CREATE_NEW
    res = f_open(&file, filename, FA_WRITE|FA_CREATE_ALWAYS);
    if (res == FR_OK)
    {
        for (int i = 0 ; i < h_param->size ; i++)
        {
            param_t * p = &(h_param->list[i]);

            switch(h_param->list[i].type)
            {
            case PARAM_TYPE_FLOAT:
                snprintf(buffer, 128, "%s=%0.2f\n",p->name, *(float*)p->value.fval);
                f_puts(buffer, &file);
                break;
            case PARAM_TYPE_BOOL:
                snprintf(buffer, 128, "%s=%d\n",p->name, *(uint8_t*)p->value.bval);
                f_puts(buffer, &file);
                break;
            default:
                break;
            }
        }
        return f_close(&file);
    }

    return res;
}

uint8_t sd_load_param(const char *filename, h_param_t * h_param)
{
    FIL file;
	FRESULT res;
    char buffer[128];

    res = f_open(&file, filename, FA_READ);
    if (res == FR_OK)
    {
        TCHAR * ret = buffer;
        while(ret != 0)
        {
            ret = f_gets(buffer, 128, &file);

            sd_parse_line(buffer, h_param);
        }

        return f_close(&file);
    }
    return res;
}

static uint8_t sd_parse_line(const char * line, h_param_t * h_param)
{
    char * eq = strchr(line, '=');
    
    if (!eq)
    {
        return 1; // No '=' : invalid line
    }

    // Searching a dot to separate bool from float
    const char * dot = strchr(line, '.');

    // Separate param name from value to use strcmp
    *eq = '\0';

    for (int i = 0 ; i < h_param->size ; i++)
    {
        param_t * p = &(h_param->list[i]);

        // printf("comparing %s to %s : %d\r\n", line, p->name, strcmp(line, p->name));
        if(strcmp(line, p->name) == 0)
        {
            printf("found param %s=", p->name);

            if (dot)
            {
                // Float
                p->type = PARAM_TYPE_FLOAT;
                float f = strtof(eq + 1, NULL);
                printf("%0.2f\r\n", f);
                *(float*)(p->value.fval) = f;
            }
            else 
            {
                // Bool
                p->type = PARAM_TYPE_BOOL;
                uint8_t b = strtoul(eq + 1, NULL, 10);
                printf("%d\r\n", b);
                *(uint8_t*)(p->value.bval) = b;
            }
        }
    }

    return 0;
}