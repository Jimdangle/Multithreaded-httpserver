#include <string.h>
#include <stdlib.h>

#define VALIDVERSION "HTTP/1.1"

//
int farseRequest(char *s, int *method, char *uri) {
    char *crlf = strstr(s, "\r\n"); // find the first break because that is all we care about
    if (crlf == NULL) {
        return -1;
    }

    // we have a crlf in this string s
    int pos = crlf - s; // get the index of the \r\n

    char *line = strndup(s, pos); //store the line

    char *met = (char *) calloc(20, sizeof(char));
    ; // lil char array to store method
    char *vers = (char *) calloc(20, sizeof(char));
    ; // lil char array to store version
    int args = sscanf(s, "%s %s %s", met, uri, vers);

    if ((args < 3) || (strncmp(vers, VALIDVERSION, 8) != 0)
        || (uri[0] != '/')) // Not enough arguments / string did  not match this pattern of 3 spaces
    {
        //printf("scanf\n");
        free(met);
        free(vers);
        free(line);
        return -1;
    }

    for (size_t i = 0; i < strlen(met); i++) {
        if (isalpha(met[i]) == 0) // not an alphabetical character so can not be a method we support
        {
            //printf("non-alph method char\n");
            free(line);
            free(met);
            free(vers);
            return -1;
        }
    }

    for (size_t i = 1; i < strlen(uri); i++) {
        if (isalnum(uri[i]) != 0 || uri[i] == '.' || uri[i] == '/' || uri[i] == '_') {

        } else {
            free(met);
            free(vers);
            free(line);
            return -1; // invalid method character format
        }
    }

    if (strncmp(met, "GET", 3) == 0) // set an int for a method we support
    {
        *method = 1;
    }
    if (strncmp(met, "PUT", 3) == 0) {
        *method = 2;
    }
    if (strncmp(met, "APPEND", 7) == 0) {
        *method = 3;
    }
    free(met);
    free(vers);
    free(line); // we fall thru
    return 1; // return 1 we set method and freed things
}

int farseHeaders(char *s, int *cl, int *rid) {
    char *crlf, *dcrlf; // crlf and double (end of headers)
    crlf = strstr(s, "\r\n");
    dcrlf = strstr(s, "\r\n\r\n");

    char *c;

    int size = 0;
    if (crlf == NULL && dcrlf == NULL) {
        //printf("crlf\n");
        return -1;
    }
    if (crlf == NULL && dcrlf != NULL) {
        size = dcrlf - s;
        c = s;
    } else {
        size = dcrlf - crlf;
        c = crlf + 2;
    }

    char *h = strndup(c, size - 1);

    // basically take in the whole buffer, get only the headers the run a regex patter on each header to make sure its valid
    // also use this time to strcmp(Content-Lenght: and )

    char *cont = NULL;
    char *tokens = strtok_r(h, "\r\n", &cont);

    while (tokens != NULL) {
        char *t = strdup(tokens);

        char *hc = NULL;
        char *subtok = strtok_r(t, ":", &hc);
        // first should be key
        for (size_t i = 0; i < strlen(subtok); i++) {
            if (isascii(subtok[i]) == 0) {
                return -1;
            }
        }

        //shifty shifty
        char *cls = strstr(tokens, "Content-Length:"); // set content length if its here
        if (cls != NULL) {
            *cl = atoi(cls + strlen("Content-Length:"));
        }
        char *rids = strstr(tokens, "Request-Id:"); // set content length if its here
        if (rids != NULL) {
            //printf("found req id\n");
            *rid = atoi(rids + strlen("Request-Id:"));
            //printf("set req id\n");
        }
        free(t);

        tokens = strtok_r(NULL, "\r\n", &cont);
    }

    free(h);

    return 1;
}
