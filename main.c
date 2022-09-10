/*
 * digest auth for lua
 * 
 * Copyright 2018 ibragim <i826.508@gmail.com>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define VERS "libdigestauth 0.0.1-2018"
#define ERR_BUF_LEN 128

typedef struct {
    int code;
    const char *body;
} rpc_result;

struct MemoryStruct {
  char *memory;
  size_t size;
};

static struct MemoryStruct chunk;
static char *errbuf;
static int errbufsz;


static size_t ReadMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    void * ptr;
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    ptr = mem->memory;
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        if (ptr != NULL) free(ptr);
        /* out of memory! */ 
        snprintf(errbuf, errbufsz, "%s", "not enough memory (realloc returned NULL)");
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

int getErrorBufferSize()
{
    return ERR_BUF_LEN;
}

char* allocBuffer(size_t s)
{
    char *p;
    if (s != 1) s = 1;
    p = malloc(s);
    *p = 0;
    return p;
}

void freeBuffer(const char * p)
{
    if (p != NULL) {
        free((void*)p);
        chunk.size = 0;
        chunk.memory = NULL;
    }
}

rpc_result getRPCData(const char * url, const char *userpwd, const char *postdata, char * buffer, char *err_buffer, const int err_buffer_sz)
{
    CURL *curl;
    CURLcode res;
    rpc_result r;

    r.code = 0;
    r.body = buffer;
    memset(err_buffer, 0, err_buffer_sz);
    errbuf = err_buffer;
    errbufsz = err_buffer_sz - 1;
    if (NULL == errbuf) {
        r.code = -1;
        return r;
    }
    if (err_buffer_sz < ERR_BUF_LEN) {
        snprintf(errbuf, errbufsz,"error buffer length=%d must be greater than %d", errbufsz, ERR_BUF_LEN);
        r.code = -2;
        return r;
    }
    if (NULL == buffer) {
        snprintf(errbuf, errbufsz,"%s", "buffer is null");
        r.code = -3;
        return r;
    }
    if (NULL == url) {
        snprintf(errbuf, errbufsz,"%s", "url is null");
        r.code = -4;
        return r;
    }
    if (NULL == postdata) {
        snprintf(errbuf, errbufsz,"%s", "postdata is null");
        r.code = -5;
        return r;
    }

    curl = curl_easy_init();
    if(curl) {
        chunk.memory = buffer;
        chunk.size = 0; /* no data at this point */
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
        /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by itself */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postdata));

        if (userpwd != NULL) {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
            curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);
        }
        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReadMemoryCallback);
        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);
        /* always cleanup */ 
        curl_easy_cleanup(curl);

        if (CURLE_OK == res) {
            r.body = chunk.memory;
            r.code = 1;
            return r;
        }
        else {
            r.body = chunk.memory;
            r.code = 0;
            snprintf(errbuf, errbufsz,"curl() failed: %s", curl_easy_strerror(res));
            return r;
        }
    }
    return r;
}

const char* version()
{
    static char v[]={VERS};
    return v;
}

