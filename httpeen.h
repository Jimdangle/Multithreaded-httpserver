#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFF_SIZE 4096

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);

void lerg(
    int op, char *uri, int code, int uid); // gonna use this to format what gets written to the log

int writeToURI(int connfd, int fd, int cl);

// HTTP Methods
int getM(int connfd, char *uri, int uid) {
    char buffer[BUFF_SIZE];
    char *s; // status line

    //printf("GET CALLEd\n");
    //printf("GET GOT\n"); // got to get
    int op = open(uri, O_RDONLY); // try to open
    if (op < 0) // handle error cases
    {
        if (errno == ENOENT) //404
        {
            return 404;
        }
        if (errno == EACCES || errno == EISDIR) //403
        {
            return 403;
        }
    }

    //printf("Opened  %s from get\n", uri );
    ssize_t bytez = read(op, buffer, BUFF_SIZE);

    if (bytez < 0) // handle read errors
    {
        if (errno == ENOENT) // 404
        {
            return 404;
        }
        if (errno == EACCES || errno == EISDIR) //403
        {
            return 403;
        }
    }

    struct stat uriStat;
    if (stat(uri, &uriStat) == -1) // couldnt get file size
    {
        return 500;
    }

    char *cl = (char *) calloc(100, sizeof(char));
    int cle = snprintf(cl, 100, "Content-Length: %ld\r\n\r\n", uriStat.st_size);

    if (cle < 0) { // could not format string
        free(cl);
        return 500;
    }

    s = "HTTP/1.1 200 OK\r\n";
    write(connfd, s, strlen(s));
    write(connfd, cl, strlen(cl));

    while (bytez > 0) // im just not even gonna worry bout write errors to the socket
    {
        write(connfd, buffer, bytez);
        bytez = read(op, buffer, bytez);
    }

    close(op);

    lerg(1, uri, 200, uid); // log it
    free(cl);

    return 1; // successs
}

int putM(int connfd, char *uri, int cl, int uid, int more, char *b) {
    //printf("C:%d | U:%s | CL:%d | UI:%d | M:%d | B:%s\n",connfd,uri,cl,uid,more,b);

    int filed; // filedescriptor
    char *s;

    int status = 0;

    struct stat fi; // file stat
    if (stat(uri, &fi) == 0) // file exissts
    {
        filed = open(uri, O_WRONLY | O_TRUNC, 0644); //
        if (filed < 0) { // handle errors in opening for wronly
            if (errno == EACCES || errno == EISDIR || errno == EROFS) {
                return 403;
            }
        }
        status = 200;
        s = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOk\n";

    } else { // file does not exist
        filed = open(uri, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (filed < 0) {
            if (errno == EACCES || errno == EISDIR || errno == EROFS) {
                return 403;
            }
        }
        status = 201;
        s = "HTTP/1.1 201 CREATED\r\nContent-Length: 8\r\n\r\nCreated\n";
    }

    int writ = 0;
    if (more > 0) {
        //printf("writing in init %s | %lu\n",b, strlen(b));
        writ = write(filed, b, more);
    }

    if (cl - writ > 0) {
        writeToURI(connfd, filed, cl - writ);
    }
    close(filed);

    write(connfd, s, strlen(s));
    lerg(2, uri, status, uid); // log it

    return 1;
}

int appendM(int connfd, char *uri, int cl, int uid, int more, char *b) {
    //printf("C:%d | U:%s | CL:%d | UI:%d | M:%d | B:%s\n",connfd,uri,cl,uid,more,b);

    int filed;
    char *s;

    struct stat fi; // file stat
    if (stat(uri, &fi) == 0) // file exissts
    {
        filed = open(uri, O_WRONLY | O_APPEND); //open for append
        if (filed < 0) { // handle errors in opening for wronly
            if (errno == EACCES || errno == EISDIR || errno == EROFS) {
                return 403;
            }
        }

        s = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOk\n";
    } else {
        return 404;
    }

    int writ = 0;
    if (more > 0) {
        //printf("writing in init %s | %lu\n",b, strlen(b));
        writ = write(filed, b, more);
    }

    if (cl - writ > 0) {
        writeToURI(connfd, filed, cl - writ);
    }
    close(filed);

    write(connfd, s, strlen(s));
    lerg(3, uri, 200, uid); // log it

    return 1;
}

int writeToURI(int connfd, int fd, int cl) {

    char *buff = (char *) calloc(BUFF_SIZE, sizeof(char));
    ;
    size_t bytesRead = read(connfd, buff, BUFF_SIZE);

    int left = cl;

    // want to write cl bytes to fd from connfd
    while (bytesRead > 0 && left > 0) {
        //printf("writing after init\n");
        int toWrite = (left < (int) bytesRead)
                          ? left
                          : bytesRead; // write either what we need left or max amount we can
        write(fd, buff, toWrite); // write
        left = left - toWrite; // subtract what we wrote to the file
        if (left <= 0) {
            break;
        } // we did all of it
        bytesRead = read(connfd, buff, BUFF_SIZE);
    }

    //printf("done writing\n");
    //write(connfd,s,strlen(s));
    free(buff);
    return 1;
}

char *genResponse(int statusCode, int connfd) {

    char *m = NULL;
    switch (statusCode) {
    case 400: m = "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n"; break;
    case 403: m = "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n"; break;
    case 404: m = "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n"; break;
    case 500:
        m = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 23\r\n\r\nInternal Server "
            "Error\n";

        break;
    case 501:
        m = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n";
        break;
    case 200: m = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOk\n"; break;
    case 201: m = "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\rn"; break;
    default: // not a code so dont do anything
        return NULL;
        break;
    }

    write(connfd, m, strlen(m));

    return m;
}

void lerg(
    int op, char *uri, int code, int uid) // gonna use this to format what gets written to the log
{
    char *oper;
    switch (op) // set the operation string based on input int op
    {
    case 1: oper = "GET"; break;
    case 2: oper = "PUT"; break;
    case 3: oper = "APPEND"; break;
    }

    LOG("%s,/%s,%d,%d\n", oper, uri, code, uid);
    fflush(logfile); // flush it cause thats whaat everyone said to  do and im a lemur
}
