#include <arpa/inet.h>
#include <dlfcn.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define LIBC_SONAME "libc.so.6"
#define LOG_FILE "log"
#define BUFF_LEN 1024

FILE *(*old_fopen)(const char *path, const char *mode) = NULL;
int (*old_fclose)(FILE *stream) = NULL;
int (*old_fputs)(const char *s, FILE *stream) = NULL;
int (*old_getaddrinfo)(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) = NULL;
int (*old_connect)(int socket, const struct sockaddr *address, socklen_t address_len) = NULL;

char *logmessage = NULL;

void myprintf(char *message) {
    if (old_fopen == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
            old_fopen = dlsym(handle, "fopen");
            dlclose(handle);
        }
    }
    if (old_fclose == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
            old_fclose = dlsym(handle, "fclose");
            dlclose(handle);
        }
    }
    FILE *hijacklog = old_fopen(LOG_FILE, "a");
    fprintf(hijacklog, message);
    old_fclose(hijacklog);
    return;
}

FILE *fopen(const char *path, const char *mode){
    if (old_fopen == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
            old_fopen = dlsym(handle, "fopen");
            dlclose(handle);
        }
    }
    FILE *targetfile = NULL;
    if (logmessage == NULL) logmessage = (char*)malloc(BUFF_LEN);
    sprintf(logmessage, "Openfile: \"%s\" \"%s\"", path, mode);
    myprintf(logmessage);
    if (old_fopen != NULL) {
        targetfile = old_fopen(path, mode);
        sprintf(logmessage, " %p\n", targetfile);
        myprintf(logmessage);
        return targetfile;
    }
    else return NULL;
}


int fputs(const char *s, FILE *stream){
    if (old_fputs == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
            old_fputs = dlsym(handle, "fputs");
            dlclose(handle);
        }
    }
    if (logmessage == NULL) logmessage = (char*)malloc(BUFF_LEN);
    sprintf(logmessage, "Write: %p\n", stream);
    myprintf(logmessage);
    if (old_fputs != NULL) return old_fputs(s, stream);
    else return 0;
}


int fclose(FILE *stream){
    if (old_fclose == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
            old_fclose = dlsym(handle, "fclose");
            dlclose(handle);
        }
    }
    if (logmessage == NULL) logmessage = (char*)malloc(BUFF_LEN);
    sprintf(logmessage, "Closefile: %p\n", stream);
    myprintf(logmessage);
    if (old_fclose != NULL) return old_fclose(stream);
    else return 0;
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res) {
    if (old_getaddrinfo == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
          old_getaddrinfo = dlsym(handle, "getaddrinfo");
          dlclose(handle);
        }
    }
    if (logmessage == NULL) logmessage = (char*)malloc(BUFF_LEN);
    sprintf(logmessage, "Resolve: \"%s\"\n", node);
    myprintf(logmessage);
    if (old_getaddrinfo != NULL)
        return old_getaddrinfo(node, service, hints, res);
    else return 0;
}

int connect(int socket,
            const struct sockaddr *address,
            socklen_t address_len) {
    if (old_connect == NULL) {
        void *handle = dlopen(LIBC_SONAME, RTLD_LAZY);
        if(handle != NULL) {
            old_connect = dlsym(handle, "connect");
            dlclose(handle);
        }
    }
    if (logmessage == NULL) logmessage = (char*)malloc(BUFF_LEN);
    struct sockaddr_in *address_in = (struct sockaddr_in*)address;
    sprintf(logmessage, "Connect: %s\n", inet_ntoa(address_in->sin_addr));
    myprintf(logmessage);
    if (old_connect != NULL) return old_connect(socket, address, address_len);
    else return 0;
}
