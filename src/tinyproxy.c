#include "tinyproxy.h"
#define NPROC 10

int main(int argc, char **argv)
{
    int listenfd, connfd, clientlen;
    struct sockaddr_in clientaddr;
    pid_t pid;
    int i;

    Signal(SIGCHLD, handler);
    Signal(SIGINT, handler);

    //PARTIE 2.2 redirection vers proxy
    if(argc == 5)
    {
        if (strcmp(argv[2], "-p") == 0)
        {
            proxyIP = argv[3];
            proxyPort = atoi(argv[4]);
        }
        else
        {
            fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
            exit(1);
        }
    }

    clientPort = atoi(argv[1]);
    listenfd = Open_listenfd(clientPort);

    //PARTIE 2.3 Proxy concurrent
    for(i=0; i<NPROC; i++){
        pid = Fork();
        if(pid==0){
            while (1){
                clientlen = sizeof(clientaddr);
                connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t*)&clientlen);
                //connexion success avec client
                if(connfd >= 0){
                    doit(connfd);
                }
                //fermeture connexion avec client
                Close(connfd);
            }
        }
    }

    while(1){
		sleep(1);
	}

    return 0;
}


// Handle one HTTP request/response transaction
void doit(int clientfd)
{

///////////////////////////////
    /*      REQUETE CLIENT   */
    ///////////////////////////

    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];     // buf = requestClient
    rio_t rio_client, rio_serveur;
    ssize_t r;
    int c;
    int serverfd;

    /* Read request line and headers */
    Rio_readinitb(&rio_client, clientfd);
    r = Rio_readlineb(&rio_client, buf, MAXLINE);
    if (r == 0) {
        /* the client left without sending a request */
        printf("received empty request\n");
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("-------- REQUETE CLIENT ---------\n");
    printf("%s %s %s\n", method, uri, version);

    //Erreur si methode n'est pas du type GET
    if (strcasecmp(method, "GET")) {
        clienterror(clientfd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    // ALLOCATION MEMOIRE ET STOCKAGE DE LA LIGNE DE REQUETE
    requeteClient = (char*) Malloc(sizeof(char) * MAXLINE);
    strcpy(requeteClient, buf);
    // headerClient = strcat(headerClient, requestClient);

    // LECTURE HEADER REQUETE CLIENT 
    c = read_requesthdrs(&rio_client);
    if (c == 0) {
        printf("incomplete request - ignored\n");
        return;
    }
    // AFFICHAGE REQUETE CLIENT 
    printf("\n-------- CONTENU REQUETE ---------\n%s", requeteClient);


    // RECUPERATION INFOS SERVEUR
    strtok(hostLine, " ");
    serveurIP = strtok(NULL, ":");
    char* portstr;
    if((portstr = strtok(NULL, ":"))!=NULL){
        serveurPort = atoi(portstr);
    }else{
        serveurIP[strlen(serveurIP)-2] = '\0';
        serveurPort = 80;
    }


/////////////////////////////////
        /*    REQUETE SERVEUR  */
        ///////////////////////// 

    // CHECK SI IP DANS BLACKLIST
	int filtre = filtrageRequete(serveurIP);

    // SI IL N'Y EST PAS, ECHANGE AVEC SERVEUR + ENVOI AU CLIENT
	if(filtre == 0){

        // CONNEXION AVEC PROXY OU SERVEUR
        if(proxyIP != NULL){
            serverfd = Open_clientfd(proxyIP, proxyPort);
		}else{
			serverfd = Open_clientfd(serveurIP, serveurPort);
		}

        // CONNEXION AVEC SERVEUR
		Rio_readinitb(&rio_serveur, serverfd);
		Rio_writen(serverfd, requeteClient, strlen(requeteClient));

        // meme technique que pour recuperer la requete du client !
        char buf2[MAXLINE], version2[MAXLINE], code_reponse2[MAXLINE], text_reponse2[MAXLINE];
        ssize_t r2;
        int c2;

        //lecture premiere ligne de la reponse serveur
        r2 = Rio_readlineb(&rio_serveur, buf2, MAXLINE);
        if (r2 == 0) {
            /* the client left without sending a request */
            printf("received empty request\n");
            return;
        }
        sscanf(buf2, "%s %s %s", version2, code_reponse2, text_reponse2);

        printf("-------- REPONSE SERVEUR ---------\n");
        printf("%s %s %s \n", version2, code_reponse2, text_reponse2);

        //decoupage de la premiere ligne et recuperation des infos qui nous interesse dans le journal
        strtok(buf2, " ");
        int code = atoi(strtok(NULL, " "));
        journal(serveurIP, code);

        // sauvegarde de la premiere ligne
        requeteServeur = (char*) Malloc(sizeof(char) * MAXLINE);
        strcpy(requeteServeur, buf2);

        // lecture header reponse serveur
        c2 = read_requesthdrs2(&rio_serveur);
        if (c2 == 0) {
            printf("incomplete request - ignored\n");
            return;
        }

        // ENVOI DE LA REQUETE SERVEUR AU CLIENT
		Rio_writen(clientfd, requeteServeur, strlen(requeteServeur));    

        // FERMETURE CONNEXION AVEC SERVEUR
		Close(serverfd);

	}else{
		clienterror(clientfd,"Access forbiden", "404", "Blacklist Server", "");
		printf("Access forbiden : blacklist server\n");
	}


}

int read_requesthdrs(rio_t *rp)
{
    char* buf;
    ssize_t r;
    int compteur = 1;

    hostLine = (char*) Malloc(sizeof(char) * MAXLINE);
    buf = (char*) Malloc(sizeof(char) * MAXLINE);


    r = Rio_readlineb(rp, buf, MAXLINE);
    if (r == 0) {
        return 0;
    }

    compteur = compteur + 1;
    requeteClient = Realloc(requeteClient, MAXLINE * compteur);
    requeteClient = strcat(requeteClient, buf);

    // hostline = premiere ligne de la requete 
    strcpy(hostLine, buf);

    // lecture de chaque ligne de la requete client une par une et on la sauvegarde dans requeteClient en reallouant de la memoire pour chaque nouvelle ligne 
    while (strcmp(buf, "\r\n")) {
        r = Rio_readlineb(rp, buf, MAXLINE);
        if (r == 0) {
         return 0;
        }

        compteur = compteur + 1;
        requeteClient = Realloc(requeteClient, MAXLINE * compteur);

        //suppression encoding car probleme lors de l'affichage
        if(strstr(buf, "Accept-Encoding")!= NULL){
			strcpy(buf,"Accept-Encoding: \n");
	   }

        requeteClient = strcat(requeteClient, buf);
    }

    return 1;
}



int read_requesthdrs2(rio_t *rio_serveur)
{
    int compteur = 1;
    ssize_t r;

    r = Rio_readlineb(rio_serveur, requeteClient, MAXLINE);
    if (r == 0) {
         /* the serveur sent an incomplete request */
        return 0;
    }

    compteur = compteur + 1;
    requeteClient = Realloc(requeteClient, MAXLINE * compteur);
    requeteClient = strcat(requeteClient, requeteClient);

    // tant que les reponses serveurs ne sont pas vides, on sauvegarde dans requeteServeur en reallouant de la memoire pour chaque nouvelle ligne 
    while((r = Rio_readlineb(rio_serveur, requeteClient, MAXLINE)) > 0){
        if (r == 0) {
            /* the serveur sent an incomplete request */
            return 0;
        }
        compteur = compteur + 1;
        requeteServeur = Realloc(requeteServeur, MAXLINE * compteur);
        requeteServeur = strcat(requeteServeur, requeteClient);
     }

    return 1;
}




void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf1[MAXLINE];
    char body1[MAXBUF], body2[MAXBUF];

    /* Build the HTTP response body */
    snprintf(body1, sizeof(body1), "<html><title>Tiny Error</title>");
    snprintf(body2, sizeof(body2), "%s<body bgcolor=""ffffff"">\r\n", body1);
    snprintf(body1, sizeof(body1), "%s%s: %s\r\n", body2, errnum, shortmsg);
    snprintf(body2, sizeof(body2), "%s<p>%s: %s\r\n", body1, longmsg, cause);
    snprintf(body1, sizeof(body1), "%s<hr><em> TinyProxy </em>\r\n", body2);

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



