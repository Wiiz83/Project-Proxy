#include "tinyproxy.h"

// Fonction pour sauvegarder l'historique de recherche (qu'elles soient fructueuses ou non)
void journal(char* url, int code){
    char buffer[MAXLINE];
    time_t temps;
    struct tm * date;

	// Configurations des informations temporelles
    time (&temps); 
    date = localtime(&temps);
    strftime(buffer,32,"%a %d %b %Y %H:%M:%S",date); 

    // Ecriture des elements dans le fichier journal 
    FILE *file = fopen("journal", "a"); 
    fprintf(file, "%s %s %s %d\n", buffer, inet_ntoa(clientaddr.sin_addr), url, code);  

    fclose(file); 
}
