/*
https://github.com/solusipse/ureq

The MIT License (MIT)

Copyright (c) 2015-2016 solusipse

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef UREQ_FILESYSTEM_H
#define UREQ_FILESYSTEM_H

/*
   This function comes from dht-esp8266 project
   It was written by Jeroen Domburg <jeroen@spritesmods.com>

   Slightly modified for this project
*/
static void memcpy_aligned(char *dst, char *src, const int len) {
    int x, w, b;
    for (x = 0; x < len; ++x, ++dst, ++src) {
        b = ((int)src & 3);
        w = *((int *)(src - b));
        switch (b) {
            case 0 : *dst=(w>>0);  break;
            case 1 : *dst=(w>>8);  break;
            case 2 : *dst=(w>>16); break;
            case 3 : *dst=(w>>24); break;
        }
    }
}

static char *ureq_fs_read(const int a, const int s, char *buf) {
    char *pos = (char*)(UREQ_FS_START + 0x40200000);
    pos += a;
    memset(buf, 0, 1024);
    memcpy_aligned(buf, pos, s);
    return buf;
}

static UreqFile ureq_fs_open(const char *rf) {
    char *pos = (char*)(UREQ_FS_START + 0x40200000);
    char    name[16];
    int32_t size;
    int32_t address;

    UreqFile f = {0, 0};

    /* Get number of files */
    int32_t amount;
    os_memcpy(&amount, pos, sizeof(amount));

    /* Move to the filesystem header */
    pos += sizeof(int32_t);

    int i;
    for (i = 0; i < amount; ++i) {
        memset(name, 0, sizeof(name));
        os_memcpy(name, pos, sizeof(name));

        if (strcmp(name, rf) == 0) {
            /* Requested file was found */
            pos += sizeof(char) * 16;
            os_memcpy(&size, pos, sizeof(int32_t));
            pos += sizeof(int32_t);
            os_memcpy(&address, pos, sizeof(int32_t));
            f.size = size;
            f.address = address;
            return f;
        } else {
            /* Move to next file */
            pos += sizeof(char) * 16;   /* filename */
            pos += sizeof(int32_t);     /* size */
            pos += sizeof(int32_t);     /* address */
        }
    }
    return f;
}

static int ureq_fs_first_run(HttpRequest *r) {
    /* 
       If there's no function bound to /, then try
       to read index.html
    */
    UreqFile f;

    if (r->response.file) {
        f = ureq_fs_open(r->response.file);
    } else {
        if (strcmp(r->url, "/") == 0)
            f = ureq_fs_open("index.html");
        else
            f = ureq_fs_open(r->url + 1);
    }

    if (f.address == 0) {
        /* File was not found */
        return ureq_set_404_response(r);
    }

    if (r->response.code == 0)
        r->response.code = 200;

    r->file      =  f;
    r->big_file  =  1;
    r->complete  = -2;
    r->response.data = ureq_generate_response_header(r);
    r->len = strlen(r->response.data);
    return r->complete;
}

#endif /* UREQ_FILESYSTEM_H */
