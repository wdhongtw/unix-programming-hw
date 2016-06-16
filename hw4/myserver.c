#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <regex.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define HELLO_MSG "Hello World!!\n"

void sendstr(int client, char* str) {
    send(client, str, strlen(str), 0);
    return;
}

void sendmeta(int client, char *filename) {
    int file;
    char size_str[1000];
    off_t size;
    regex_t regex;
    size_t nmatch = 1;
    regmatch_t pmatch[1];
    char *ext;
    size_t ext_len;

    if (regcomp(&regex, "\\.([^./]*)$", REG_EXTENDED)!=0) {
        fprintf(stderr, "regex compile error\n");
        exit(EXIT_FAILURE);
    }
    if (regexec(&regex, filename, nmatch, pmatch, 0)!=0) {
        fprintf(stderr, "Check ext of %s\n", filename);
        fprintf(stderr, "regex exec fail\n");
        exit(EXIT_FAILURE);
    }
    ext_len = pmatch[0].rm_eo-pmatch[0].rm_so;
    ext = (char *)calloc(ext_len, sizeof(char));
    strncpy(ext, filename+pmatch[0].rm_so+1, ext_len);
    printf("ext: %s\n", ext);
    if ((strcmp(ext, "txt")==0)) sendstr(client, "Content-Type: text/plain\x0d\x0a");
    else if ((strcmp(ext, "html")==0)) sendstr(client, "Content-Type: text/html\x0d\x0a");
    else if ((strcmp(ext, "htm")==0)) sendstr(client, "Content-Type: text/html\x0d\x0a");
    else if ((strcmp(ext, "css")==0)) sendstr(client, "Content-Type: text/css\x0d\x0a");
    else if ((strcmp(ext, "gif")==0)) sendstr(client, "Content-Type: image/gif\x0d\x0a");
    else if ((strcmp(ext, "jpg")==0)) sendstr(client, "Content-Type: image/jpg\x0d\x0a");
    else if ((strcmp(ext, "png")==0)) sendstr(client, "Content-Type: image/png\x0d\x0a");
    else if ((strcmp(ext, "bmp")==0)) sendstr(client, "Content-Type: image/x-ms-bmp\x0d\x0a");
    else if ((strcmp(ext, "doc")==0)) sendstr(client, "Content-Type: application/msword\x0d\x0a");
    else if ((strcmp(ext, "pdf")==0)) sendstr(client, "Content-Type: application/pdf\x0d\x0a");
    else if ((strcmp(ext, "swf")==0)) sendstr(client, "Content-Type: application/x-shockwave-flash\x0d\x0a");
    else if ((strcmp(ext, "swfl")==0)) sendstr(client, "Content-Type: application/x-shockwave-flash\x0d\x0a");
    else if ((strcmp(ext, "bz2")==0)) sendstr(client, "Content-Type: application/x-bzip2\x0d\x0a");
    else if ((strcmp(ext, "gz")==0)) sendstr(client, "Content-Type: application/x-gzip\x0d\x0a");
    else if ((strcmp(ext, "tar.gz")==0)) sendstr(client, "Content-Type: application/x-gzip\x0d\x0a");
    else if ((strcmp(ext, "ogg")==0)) sendstr(client, "Content-Type: audio/ogg\x0d\x0a");
    else if ((strcmp(ext, "mp4")==0)) sendstr(client, "Content-Type: video/mp4\x0d\x0a");

    file = openat(AT_FDCWD, filename, O_RDONLY);
    if (file==-1) { perror("open"); fprintf(stderr, "%s\n", filename); }
    size = lseek(file, (off_t)0, SEEK_END);
    if (size==-1) { perror("lseek"); }
    snprintf(size_str, 1000, "Content-Length: %d\x0d\x0a", (int) size);
    sendstr(client, size_str);

    free(ext);
    close(file);
    return;
}

void sendfile(int client, char *filename) {
    int file;
    off_t size;
    char *buf;

    file = openat(AT_FDCWD, filename, O_RDONLY);
    size = lseek(file, (off_t)0, SEEK_END);
    if (size==-1) { perror("lseek"); }
    lseek(file, (off_t)0, SEEK_SET);
    buf = (char *)malloc((size_t)size);
    read(file, buf, size);
    send(client, buf, size, 0);
    free(buf);
    close(file);
    return;
}

void respond_403(int client) {
    char filename[]="403.html";

    sendstr(client, "HTTP/1.1 403 Forbidden\x0d\x0a");
    sendmeta(client, filename);
    sendstr(client, "\x0d\x0a");
    sendfile(client, filename);
    return;
}

void respond_404(int client) {
    char filename[]="404.html";

    sendstr(client, "HTTP/1.1 404 Forbidden\x0d\x0a");
    sendmeta(client, filename);
    sendstr(client, "\x0d\x0a");
    sendfile(client, filename);
    return;
}

void respond_file(int client, char *filename) {
    sendstr(client, "HTTP/1.1 200 OK\x0d\x0a");
    sendmeta(client, filename);
    sendstr(client, "\x0d\x0a");
    sendfile(client, filename);
    return;
}

void respond_301(int client, char *dirname) {
    char new_link[256];

    sprintf(new_link, "Location: /%s/\x0d\x0a", dirname);
    sendstr(client, "HTTP/1.1 301 Moved Permanently\x0d\x0a");
    sendstr(client, new_link);
    sendstr(client, "\x0d\x0a");
    return;
}

void respond_dir(int client, char *dirname) {
    FILE *dir;
    char filelist[2048];
    char command[256];

    sprintf(command, "/bin/ls -l ./%s", dirname);
    dir = popen(command, "r");

    sendstr(client, "HTTP/1.1 200 OK\x0d\x0a");
    sendstr(client, "\x0d\x0a");
    while (fgets(filelist, 2046, dir) != NULL) {
        sendstr(client, filelist);
    }

    fclose(dir);
    return;
}

void respond(int client, char *resource) {
    struct stat res_stat;
    stat(resource, &res_stat);
    char index_file[256];
    char res_len;

    resource = strtok(resource, "?");

    if (S_ISDIR(res_stat.st_mode)) {
        res_len = strlen(resource);
        if (resource[res_len-1]!='/') {
            respond_301(client, resource);
        } else {
            sprintf(index_file, "%sindex.html", resource);
            if (access(index_file, R_OK)==-1) {
                if (errno==ENOENT) {
                    if (access(resource, R_OK)==-1) {
                        if (errno==EACCES) { respond_404(client); return; }
                    } else {
                        respond_dir(client, resource); return;
                    }
                }
                if (errno==EACCES) { respond_403(client); return; }
            }
            respond_file(client, index_file); return;
        }
    } else if (access(resource, R_OK)==-1) {
        if (errno==ENOENT) { respond_403(client); return; }
        else if (errno==EACCES) { respond_404(client); return; }
    } else
        respond_file(client, resource);
    return;
}

void serv_client(int fd, struct sockaddr_in *sin) {
    int len;
    char buf[2048];
    regex_t regex;
    size_t nmatch = 3;
    regmatch_t pmatch[3];
    char *method, *resource;
    size_t method_len, resource_len;

    if (regcomp(&regex, "(GET|POST) /([^ ]+) HTTP/.*", REG_EXTENDED)!=0) {
        fprintf(stderr, "regex compile error\n");
        exit(EXIT_FAILURE);
    }

    printf("connected from %s:%d\n",
        inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
    len = recv(fd, buf, sizeof(buf), 0);

    if (regexec(&regex, buf, nmatch, pmatch, 0)!=0) {
        fprintf(stderr, "regex compile error\n");
        exit(EXIT_FAILURE);
    }
    method_len = pmatch[1].rm_eo-pmatch[1].rm_so+1;
    resource_len = pmatch[2].rm_eo-pmatch[2].rm_so+1;
    method = (char *)calloc(method_len, sizeof(char));
    resource = (char *)calloc(resource_len, sizeof(char));
    strncpy(method, buf+pmatch[1].rm_so, method_len-1);
    strncpy(resource, buf+pmatch[2].rm_so, resource_len-1);

    printf("Method: %s, Resource: %s, len: %d\n", method, resource, len);
    respond(fd, resource);

    printf("disconnected from %s:%d\n",
        inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));

    free(method);
    free(resource);
    return;
}

int main(int argc, char *argv[]) {
    pid_t pid;
    int fd, pfd, val;
    unsigned int uval;
    short port;
    struct sockaddr_in sin, psin;
    char *docroot;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port> </path/to/docroot>\n", argv[0]);
        exit(EXIT_FAILURE);
    } else {
        port = atoi(argv[1]);
        docroot = argv[2];
    }

    if (chdir(docroot)!=0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    val = 1;
    if (setsockopt(fd,
            SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (bind(fd, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(fd, SOMAXCONN) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while (true) {
        uval = sizeof(psin);
        bzero(&psin, sizeof(psin));
        if ((pfd=accept(fd, (struct sockaddr*) &psin, &uval))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        if ((pid = fork()) < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            /* child */
            close(fd);
            serv_client(pfd, &psin);
            shutdown(pfd, SHUT_RDWR);
            exit(0);
        }
        /* parent */
        close(pfd);
    } // end while

    return 0;
}
