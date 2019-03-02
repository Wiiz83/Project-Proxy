#ifndef __TINYPROXY_H__
#define __TINYPROXY_H__

#include "csapp.h"

// Serveur
char* hostLine;
char* serveurIP;
int serveurPort;

// Client
int clientPort;

// Proxy
char* proxyIP;
int proxyPort;

// Contenu requête HTTP client>proxy
char* requeteClient;

// Contenu requête HTTP serveur>proxy
char* requeteServeur;

//structure du socket coté client pour pouvoir l'utiliser dans l'ecriture du journal
struct sockaddr_in clientaddr;

// Handler de signaux
void handler(int sig);

// Handle one HTTP request/response transaction
void doit(int fd);

// Read and parse HTTP request headers from CLIENT (returns 1 if the request is complete, 0 otherwise)
int read_requesthdrs(rio_t *rp);

// Read and parse HTTP request headers from SERVER (returns 1 if the request is complete, 0 otherwise)
int read_requesthdrs2(rio_t *rp);

// returns an error message to the client
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// Fonction pour sauvegarder l'historique de recherche (qu'elles soient fructueuses ou non)
void journal(char* url, int code);

// Transforme un nom de domaine en adresse IP 
int hostname_to_ip(char * serveur , char* ip);

// Check si adresse est dans la blacklist "filtre"
int filtrageRequete(char* uri);


#endif
