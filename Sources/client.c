
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "client.h"

#define TRUE 1
#define FALSE 0

#define LONGUEUR_TAMPON 4096

/* Variables cachees */

/* le socket client */
int socketClient;
/* le tampon de reception */
char tamponClient[LONGUEUR_TAMPON];
int debutTampon;
int finTampon;

/* Initialisation.
 * Connexion au serveur sur la machine donnee.
 * Utilisez localhost pour un fonctionnement local.
 */
int Initialisation(char *machine) {
	return InitialisationAvecService(machine, "13214");
}

/* Initialisation.
 * Connexion au serveur sur la machine donnee et au service donne.
 * Utilisez localhost pour un fonctionnement local.
 */
int InitialisationAvecService(char *machine, char *service) {
	int n;
	struct addrinfo	hints, *res, *ressave;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(machine, service, &hints, &res)) != 0)  {
     		fprintf(stderr, "Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	ressave = res;

	do {
		socketClient = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketClient < 0)
			continue;	/* ignore this one */

		if (connect(socketClient, res->ai_addr, res->ai_addrlen) == 0)
			break;		/* success */

		close(socketClient);	/* ignore this one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("Initialisation, erreur de connect.");
     		return 0;
	}

	freeaddrinfo(ressave);
	printf("Connexion avec le serveur reussie.\n");

	return 1;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception() {
	char message[LONGUEUR_TAMPON];
	int index = 0;
	int fini = FALSE;
	int retour = 0;
	while(!fini) {
		/* on cherche dans le tampon courant */
		while((finTampon > debutTampon) && 
			(tamponClient[debutTampon]!='\n')) {
			message[index++] = tamponClient[debutTampon++];
		}
		/* on a trouve ? */
		if ((index > 0) && (tamponClient[debutTampon]=='\n')) {
			message[index++] = '\n';
			message[index] = '\0';
			debutTampon++;
			fini = TRUE;
			return strdup(message);
		} else {
			/* il faut en lire plus */
			debutTampon = 0;
			retour = recv(socketClient, tamponClient, LONGUEUR_TAMPON, 0);
			if (retour < 0) {
				perror("Reception, erreur de recv.");
				return NULL;
			} else if(retour == 0) {
				fprintf(stderr, "Reception, le serveur a ferme la connexion.\n");
				return NULL;
			} else {
				/*
				 * on a recu "retour" octets
				 */
				finTampon = retour;
			}
		}
	}
	return NULL;
}

/* Envoie un message au serveur.
 * Attention, le message doit etre termine par \n
 */
int Emission(char *message) {
	if(strstr(message, "\n") == NULL) {
		fprintf(stderr, "Emission, Le message n'est pas termine par \\n.\n");
	}
	int taille = strlen(message);
	if (send(socketClient, message, taille,0) == -1) {
        perror("Emission, probleme lors du send.");
        return 0;
	}
	printf("Emission de %d caracteres.\n", taille+1);
	return 1;
}

/* Recoit des donnees envoyees par le serveur.
 */
int ReceptionBinaire(char *donnees, size_t tailleMax) {
	int dejaRecu = 0;
	int retour = 0;
	/* on commence par recopier tout ce qui reste dans le tampon
	 */
	while((finTampon > debutTampon) && (dejaRecu < tailleMax)) {
		donnees[dejaRecu] = tamponClient[debutTampon];
		dejaRecu++;
		debutTampon++;
	}
	/* si on n'est pas arrive au max
	 * on essaie de recevoir plus de donnees
	 */
	if(dejaRecu < tailleMax) {
		retour = recv(socketClient, donnees + dejaRecu, tailleMax - dejaRecu, 0);
		if(retour < 0) {
			perror("ReceptionBinaire, erreur de recv.");
			return -1;
		} else if(retour == 0) {
			fprintf(stderr, "ReceptionBinaire, le serveur a ferme la connexion.\n");
			return 0;
		} else {
			/*
			 * on a recu "retour" octets en plus
			 */
			return dejaRecu + retour;
		}
	} else {
		return dejaRecu;
	}
}

/* Envoie des donnes au serveur en prcisant leur taille.
 */
int EmissionBinaire(char *donnees, size_t taille) {
	int retour = 0;
	retour = send(socketClient, donnees, taille, 0);
	if(retour == -1) {
		perror("Emission, probleme lors du send.");
		return -1;
	} else {
		return retour;
	}
}


/* Ferme la connexion.
 */
void Terminaison() {
	close(socketClient);
}



/* ========================================
 * Fonctions affichages
 * ======================================== */


/*
 * affiche le menu
 */
void afficheMenu() {
	printf("Choisissez une action: \n");
	printf("1 - Emission d'un fichier\n");
	printf("0 - quitter\n");
}

void gererEmission(char nomFichier[50]) {
  
 	printf("Emission du fichier %s ToDo\n", nomFichier);
	nomFichier[strlen(nomFichier)]='\n';
	Emission(nomFichier);
	
}




/* ========================================
 * Fonctions traitements
 * ======================================== */


/*
 * Renvoi un entier en fonction de la reponse reçue
 * Renvoi 0 si reponse ACK, 1 sinon
 * 2 lors de non reconnaissance du type de réponse
 */
int decodeRep(char* reponse){
  char* commande=NULL;
  char* commande2=NULL;
  char* typeRep=NULL;

  // Décodage de la réponse connexion
  
  commande = strtok(reponse,"_"); // Premiere partie de la réponse, COMMANDE
  commande2 = strtok(NULL,"_"); // Seconde partie, COMMANDE_COMMANDE
  typeRep = strtok(NULL," "); // Type : ACK ou ERR
  
  // Test
  printf("\nType Réponse : %s\n",typeRep);

  if(strcmp(typeRep, "ACK") == 0){
    return 0;}
  if(strcmp(typeRep,"ERR") == 0){
    return 1;}


  if(commande!=NULL){
    free(commande);
    commande = NULL;}
  if(commande2!=NULL){
    free(commande2);
    commande2 = NULL;}
  if(typeRep !=NULL){
    free(typeRep);
    typeRep = NULL;}

  return 2;
}




/*
 * S'occupe de la connexion de l'utilisateur -1 si erreur
 */

int connexion(){
  int resCo = 0;
  char* login=NULL; // Entrer par l'utilisateur
  char* password=NULL; // Entrer par l'utilisateur
  char* requete_temp=NULL;
  char* requete=NULL;

  // Allocation de la mémoire pour les chaines
  login = malloc((size_t)50); // (size_t) 50 caractères me parrait suffisant pour un login et mdp
  password = malloc((size_t)50);
  requete_temp = malloc((size_t)100);
  requete = malloc((size_t)100);


  // Phase utilisateur
  printf("Entrer votre login : ");
  fgets(login, 50,stdin);

  password = getpass("Mot de passe : "); // Permet de cacher les caractères tapés
 
  // Préparation de la chaine login
  login[strlen(login)-1] = '\0';

  // Création de la requête à emettre au serveur
  requete = strcat(requete,"CON_USER \0"); 
  requete_temp = strcat (login, " ");
  requete_temp = strcat(requete_temp, password);
  requete = strcat(requete, requete_temp);
  
  // Préparation de la chaine requete à l'emission => Verification : Requete OK
  requete[strlen(requete)]='\n';

  // Emission de la requete de connexion au serveur
  if(Emission(requete)==0){
    perror("\nErreur lors de l'emission de la requete de connexion");
  }

  /* printf("Avant les free"); */
  
  /* if(login != NULL ){ */
  /*   free(login); */
  /*   login = NULL; */
  /* } */

  /* if(password != NULL){ */
  /*   free(password); */
  /*   password = NULL; */
  /* } */

  /* if(requete_temp != NULL){ */
  /*   free(requete_temp); */
  /*   requete_temp = NULL; */
  /* } */

  /* if(requete != NULL){ */
  /*   free(requete); */
  /*   requete = NULL; */
  /* } */

  return resCo = 0;
}


