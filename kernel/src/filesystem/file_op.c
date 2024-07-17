#include "filesystem.h"

FILE open_file(const char* fname) {
    if(fname) {
        unsigned char device = 'a';
        char* filename = (char*)fname;

        // in all cases, if fname[1]==':' then the first character must be device letter
        if(fname[1]==':') {
            device = fname[0];
            filename += 2; // strip it from pathname
        }

        // call filesystem
        if(__FS__[device - 'a']) {
            // set volume specific information and return file
            FILE file = __FS__[device - 'a']->open(filename);
            file.device_id = device;
            return file;
        }
    }

    FILE file;
    file.flags = FS_INVALID;
    return file;
}

void read_file(FILE* file, unsigned char* buffer, unsigned int length);
void close_file(FILE* file);

void register_fs(FILESYSTEM* fs, unsigned int device_id) {
    if (device_id < FS_MAXDEV && fs)
        __FS__[device_id] = fs;
}

void unregister_fs(FILESYSTEM fs);
void unregister_fs_id(unsigned int device_id);
