/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content1[MAXLINE], content2[MAXLINE];
    int n1=0, n2=0;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p+1);
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    /* Make the response body */
    snprintf(content1, sizeof(content1), "Welcome to the Internet addition portal.\n");
    snprintf(content2, sizeof(content2), "%sThe answer is: %d + %d = %d\n",
            content1, n1, n2, n1 + n2);
    snprintf(content1, sizeof(content1), "%sThanks for visiting!\n", content2);

    /* Generate the HTTP response */
    printf("Content-length: %d\r\n", (int)strlen(content1));
    printf("Content-type: text/plain\r\n\r\n");
    printf("%s", content1);
    fflush(stdout);
    exit(0);
}
/* $end adder */
