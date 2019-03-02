/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"

void doit(int fd);
int read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, size_t bufsize, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    port = atoi(argv[1]);

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*)&clientlen);
        doit(connfd);
        Close(connfd);
    }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
    ssize_t r;
    int c;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    r = Rio_readlineb(&rio, buf, MAXLINE);
    if (r == 0) {
        /* the client left without sending a request */
        printf("received empty request\n");
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("%s %s %s\n", method, uri, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    
    c = read_requesthdrs(&rio);
    if (c == 0) {
        printf("incomplete request - ignored\n");
        return;
    }

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    if (is_static) { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

/*
 * read_requesthdrs - read and parse HTTP request headers
 *
 * returns 1 if the request is complete, 0 otherwise
 */
int read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    ssize_t r;

    r = Rio_readlineb(rp, buf, MAXLINE);
    if (r == 0) {
        /* the client sent an incomplete request */
        return 0;
    }
    printf("%s", buf);
    while (strcmp(buf, "\r\n")) {
        r = Rio_readlineb(rp, buf, MAXLINE);
        if (r == 0) {
            /* the client sent an incomplete request */
            return 0;
        }
        printf("%s", buf);
    }
    return 1;
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  /* Static content */
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else {  /* Dynamic content */
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}

/*
 * serve_static - copy a file back to the client
 */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf1[MAXBUF], buf2[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, sizeof(filetype), filetype);
    snprintf(buf1, sizeof(buf1), "HTTP/1.0 200 OK\r\n");
    snprintf(buf2, sizeof(buf2), "%sServer: Tiny Web Server\r\n", buf1);
    snprintf(buf1, sizeof(buf1), "%sContent-length: %d\r\n", buf2, filesize);
    snprintf(buf2, sizeof(buf2), "%sContent-type: %s\r\n\r\n", buf1, filetype);
    Rio_writen(fd, buf2, strlen(buf2));

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    Munmap(srcp, filesize);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, size_t bufsize, char *filetype)
{
    if (strstr(filename, ".html"))
        snprintf(filetype, bufsize, "text/html");
    else if (strstr(filename, ".gif"))
        snprintf(filetype, bufsize, "image/gif");
    else if (strstr(filename, ".jpg"))
        snprintf(filetype, bufsize, "image/jpeg");
    else
        snprintf(filetype, bufsize, "text/plain");
}

/*
 * serve_dynamic - run a CGI program on behalf of the client
 */
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* 
     * Return first part of HTTP response.
     * We assume that:
     * - Fork will not fail
     * - Execve will not fail (the existence and the permissions of the file
     *    have already been checked)
     */
    snprintf(buf, sizeof(buf), "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    snprintf(buf, sizeof(buf), "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) { /* child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */
        Execve(filename, emptylist, environ); /* Run CGI program */
    }
    Wait(NULL); /* Parent waits for and reaps child */
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
    char buf1[MAXLINE];
    char body1[MAXBUF], body2[MAXBUF];

    /* Build the HTTP response body */
    snprintf(body1, sizeof(body1), "<html><title>Tiny Error</title>");
    snprintf(body2, sizeof(body2), "%s<body bgcolor=""ffffff"">\r\n", body1);
    snprintf(body1, sizeof(body1), "%s%s: %s\r\n", body2, errnum, shortmsg);
    snprintf(body2, sizeof(body2), "%s<p>%s: %s\r\n", body1, longmsg, cause);
    snprintf(body1, sizeof(body1), "%s<hr><em>The Tiny Web server</em>\r\n", body2);

    /* Build and send the HTTP response header */
    snprintf(buf1, sizeof(buf1), "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf1, strlen(buf1));
    snprintf(buf1, sizeof(buf1), "Content-type: text/html\r\n");
    Rio_writen(fd, buf1, strlen(buf1));
    snprintf(buf1, sizeof(buf1), "Content-length: %d\r\n\r\n", (int)strlen(body1));
    Rio_writen(fd, buf1, strlen(buf1));
    /* Send the HTTP response body */
    Rio_writen(fd, body1, strlen(body1));
}
