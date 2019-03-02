#include "tinyproxy.h"


// Transforme un nom de domaine en adresse IP 
int hostname_to_ip(char * serveur , char* ip){
    struct hostent *he;
    struct in_addr **addr_list;

    // recuperer les info sur le serveur
    if( (he = gethostbyname(serveur) ) == NULL){

        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    if(addr_list[0] != NULL){
        //Retourner le premier element;
        strcpy(ip , inet_ntoa(*addr_list[0]) );
        return 0;
    }

    return 1;
}

// On lit le fichier "filtre" et on le parcours pour verifier si l'adresse du serveur (IP ou nom de domaine) est blacklistee ou non
int filtrageRequete(char* uri){
	FILE *fichier = fopen("filtre", "r");
	int uriDansLaListe = 0;
	int erreur = 0;
	char ligne[256];
	char *ipUri = malloc(sizeof(char)*11);
	while(fgets(ligne, 256, fichier)!= NULL){
		ligne[strlen(ligne)-1] = '\0';
		erreur = hostname_to_ip(uri , ipUri);
		if((strcmp(uri, ligne) == 0) || ((erreur == 0) && (strcmp(ipUri, ligne) == 0))){
			uriDansLaListe = 1;
		}

	}
	fclose(fichier);
	return uriDansLaListe;
}
