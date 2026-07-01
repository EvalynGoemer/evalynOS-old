#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <utils/panic.h>
#include <utils/globals.h>
#include <filesystem/filesystem.h>
#include <filesystem/tarfs/tarfs.h>

#define KGZ_IMPLEMENTATION
#include <kgz_singleheader.h>

// Code adapted from https://wiki.osdev.org/USTAR

unsigned char *archive = NULL;

int oct2bin(unsigned char *str, int size) {
    int n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

int tarfsRead(char* path, char* return_data, int read_length) {
    unsigned char *ptr = archive;
    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);
        if (!memcmp(ptr + 1, path, strlen(path) + 1)) {
            unsigned char *file_data = ptr + 512;
            int bytes_to_copy = (filesize < read_length) ? filesize : read_length;

            for (int i = 0; i < bytes_to_copy; i++) {
                return_data[i] = file_data[i];
            }
            return bytes_to_copy;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }
    return -1;
}

int tarfsGetFize(char* path) {
    char tarPath[256];
    tarPath[0] = '.';
    strcpy(tarPath + 1, path);

    unsigned char *ptr = archive;
    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);
        if (!memcmp(ptr, tarPath, strlen(tarPath) + 1)) {
            return filesize;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }
    return -1;
}

int tarfsWrite(__attribute__ ((unused)) char* path, __attribute__ ((unused)) char* write_data, __attribute__ ((unused))  int write_length) {
    return -1;
}

int init_tarfs() {
    // check for the module labeled as the initramfs
    if (module_request.response && module_request.response->module_count > 0) {
        for (uint64_t i = 0; i < module_request.response->module_count; i++ ) {
            if (!module_request.response->modules[i]->string) continue;
            if (strcmp(module_request.response->modules[i]->string, "initramfs") == 0) {
                const char *path = module_request.response->modules[i]->path;
                size_t len = strlen(path);
                if (len >= 3 && strcmp(path + len - 3, ".gz") == 0) {
                    printf("TARFS: initramfs is compressed with gzip\n");
                    printf("TARFS: decompressing initramfs\n");
                    archive = kgz_gzip_decompress(module_request.response->modules[i]->address, module_request.response->modules[i]->size, nullptr, nullptr); break;
                } else {
                    archive = module_request.response->modules[i]->address; break;
                }
            }
        }
    }

    // check the default initramfs path
    if (module_request.response && module_request.response->module_count > 0) {
        for (uint64_t i = 0; i < module_request.response->module_count; i++ ) {
            if (strcmp(module_request.response->modules[i]->path, "/initramfs.tar") == 0) {
                archive = module_request.response->modules[i]->address;
                goto found;
            }
            if (strcmp(module_request.response->modules[i]->path, "/initramfs.tar.gz") == 0) {
                printf("TARFS: initramfs is compressed with gzip\n");
                printf("TARFS: decompressing initramfs\n");
                archive = kgz_gzip_decompress(module_request.response->modules[i]->address, module_request.response->modules[i]->size, nullptr, nullptr); break;
                goto found;
            }
        }
    }

    found:

    if(archive == NULL) {
        panic("TARFS: Could not find initramfs.tar");
        return -1;
    }

    unsigned char *ptr = archive;

    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);
        char *path = (char*)ptr;

        if (filesize > 0) {
            struct file* file = malloc(sizeof(struct file));
            strcpy(file->path, path + 1);
            file->read = tarfsRead;
            file->write = tarfsWrite;
            register_file(file);
        }

        ptr += (((filesize + 511) / 512) + 1) * 512;
    }

    printf("TARFS: initramfs Mounted\n");
    return 0;
}
