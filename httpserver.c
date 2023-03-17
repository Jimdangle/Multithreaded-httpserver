#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <regex.h>
#include <inttypes.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "farser.h"
#include "httpeen.h"
#include "becool.h"

#define OPTIONS              "t:l:"
#define BUFF_SIZE            4096
#define DEFAULT_THREAD_COUNT 4

// Set Acceptable Version
#define VALIDVERSION "HTTP/1.1"

/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */

void handle_connection(void *connfd);

pthread_mutex_t QueueLock;
pthread_cond_t emQueue;
pthread_cond_t fQueue;

BQueue *WorkQ;

// Gonna make some thread task structs instead of passing what im passing currently
// Will probably need to make a lot of sweeping changes so maybe i should documment what im doiung
// Thread task { int connfd, char *buff}
/*
typedef struct
{
    int connfd;
    char *buffer;
} task_t;
*/

// passing handle_connection a task now
// the task is gonna get malloc'd in main, and sent to the producer to enqueue
// then when we pull it out as a consumer how do i track to free it or not :/ i think for now ill just replace the like immediate ones

/*
    1. Get connfd from main
    2. turn it into a task -> createTask(int connfd)
    3. call the producer function with the task to be placed into the work queue
    4. consumers actively waiting for tasks in the work queue
    5. consumers will act on the data held in task_t

*/
void Producer(BQueue *q, int cfd) {
    //printf("PROD CALL\n");
    pthread_mutex_lock(&QueueLock); // get that lock
    while (queue_full(q) == 1) {
        //full queue
        printf("waiting for reqs to be fufilled\n");
        pthread_cond_wait(&fQueue, &QueueLock);
    }

    task_t *task = createTask(cfd); // make a task
    enqueue(q, task); // enqueue it
    //printf("ENQ cfd: %d\n", task->connfd);
    pthread_cond_signal(&emQueue);
    pthread_mutex_unlock(&QueueLock);
}

void *Consumer(void *arg) {

    //printf("CON CALL\n");
    BQueue *queue = (BQueue *) arg;
    while (1) {
        pthread_mutex_lock(&QueueLock);
        task_t *curtask;
        while (queue_empty(queue) == 1) {
            // no items so waiting
            //printf("waiting for conns\n");
            pthread_cond_wait(&emQueue, &QueueLock);
        }

        curtask = dequeue(queue); //
        //printf("DEQ cfd: %d\n", curtask->connfd);
        pthread_cond_signal(&fQueue); // queue is no longer full
        pthread_mutex_unlock(&QueueLock);

        printf("Thread %lu handling\n", pthread_self());
        handle_connection(curtask);

        // If we have some sort of time out implemented in handle connection we could return the connfd
        // to the dispatcher to try and pick up later / but idk
        //printf("Hungry nom nom\n");
    }
    return NULL;
}

uint16_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}
/**
   Creates a socket for listening for connections.
   Closes the program and prints an error message on error.
 */
int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 500) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

void handle_connection(void *task) {
    //printf("connection : %d\n strlen of CRLF2x: %lu\n", connfd, strlen("\r\n\r\n"));

    task_t *curtask = (task_t *) task; // cast to struct

    //printf("handle_con cfd: %d\n", curtask->connfd);
    //char *buff = (char *) calloc(BUFF_SIZE, sizeof(char));
    size_t br = 0; // bytesRead
    //size_t loop = 0; // number of loop iterations (0th loop should be request line/headers)

    char URI[20]; // var for the uri
    int method = 0;

    br = read(curtask->connfd, curtask->buffer, BUFF_SIZE);
    // idk if I need loop here for read im wondering if I can just make a call to PUT and append and have the follow up read happen in there
    if (br > 0) {
        char *crlf = strstr(curtask->buffer, "\r\n");
        //printf("BR: %lu\nREAD %s", br, buff);

        if (crlf == NULL) {
            genResponse(400, curtask->connfd);
            //freeTask(curtask); // free my boi
            return;
        }
        // We are in t he first read
        int m
            = farseRequest(curtask->buffer, &method, URI); // get our method num and set the URI var

        if (m == -1) {
            genResponse(400, curtask->connfd);
            //freeTask(curtask);
            return;
        } // bad request info

        char *dcrlf = strstr(curtask->buffer, "\r\n\r\n");

        if (dcrlf == NULL) // we dont have header line yet
        {
            br = read(curtask->connfd, curtask->buffer, BUFF_SIZE); // get the header line
        }

        int conlen = 0;
        int RID = 0;
        int validHeaders = farseHeaders(curtask->buffer, &conlen, &RID);
        dcrlf = strstr(curtask->buffer, "\r\n\r\n");

        if (validHeaders == -1) {
            // i can log these later but they arent part of reqs for now
            genResponse(400, curtask->connfd);
            //freeTask(curtask);
            return;
        } // these functions handle the status code guess it coulda been done here but I wasnt sure if the status codes owuld all be the same idk im losing my mind

        if (method == 1) {
            //get(connfd,uri)
            int run = getM(curtask->connfd, URI + 1, RID);
            if (run != 1) {
                lerg(1, URI + 1, run, RID); // log
                genResponse(run, curtask->connfd);
                //freeTask(curtask); // return the status code associated with the error from get
                return;
            }
        }

        // we know it exists
        int span = br - ((dcrlf + 4) - curtask->buffer); // # of chars after final crlf

        if (span >= 1) {
            dcrlf = dcrlf + 4;
        }

        if (method == 2) {
            //put(connfd,uri)
            int run = putM(curtask->connfd, URI + 1, conlen, RID, span, dcrlf);
            if (run != 1) {
                lerg(2, URI + 1, run, RID); // log
                genResponse(run,
                    curtask->connfd); // return the status code associated with the error from get
                //freeTask(curtask);
                return;
            }
        }
        if (method == 3) {

            int run = appendM(curtask->connfd, URI + 1, conlen, RID, span, dcrlf);
            if (run != 1) {
                lerg(3, URI + 1, run, RID); // log
                genResponse(run,
                    curtask->connfd); // return the status code associated with the error from get
                //freeTask(curtask);
                return;
            }
        }
    }

    close(curtask->connfd);
    //freeTask(curtask);
    return;
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        warnx("received SIGTERM");
        fclose(logfile);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

#define CONN_SIZE 4096
// 4096 size for the queue lol?

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);

    int listenfd = create_listen_socket(port);
    //LOG("port=%" PRIu16 ", threads=%d\n", port, threads);
    // Goals
    // X 1. Make a bounded Queue to put connfds into / we could make an over flow ll but that seems like overkill
    // 2. Im still lit
    // 2.5 Producer/Consumer functions
    // 3. Thread pool (create the threads and put them into a consumer function or something? to wait around for the boundedqueue to fill up and then call handle conn in the thread?)
    //  3a. Need to develop a better understanding of our critical and non-critical sections
    // 4. rock out

    WorkQ = queue_new(CONN_SIZE);

    pthread_t workers[threads]; // get a array of threads
    pthread_mutex_init(&QueueLock, NULL);
    pthread_cond_init(&emQueue, NULL);
    pthread_cond_init(&fQueue, NULL); // init all the synch prims

    //int working[threads];

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&workers[i], NULL, Consumer, WorkQ) != 0) {
            err(EXIT_FAILURE, "failure to create thread");
        }
    }

    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        //printf("Main cfd: %d\n", connfd);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }

        Producer(WorkQ, connfd);
        //close(connfd);
    }

    for (int i = 0; i < threads; i++) {
        if (pthread_join(workers[i], NULL) != 0) {
            err(EXIT_FAILURE, "failure to join thread");
        }
    }

    queue_free(WorkQ); // free the memory
    pthread_mutex_destroy(&QueueLock);
    pthread_cond_destroy(&emQueue);
    pthread_cond_destroy(&fQueue);
    return EXIT_SUCCESS;
}
