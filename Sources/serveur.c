#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <errno.h>

#include "serveur.h"

#define TRUE 1
#define FALSE 0
#define LONGUEUR_TAMPON 4096


#ifdef WIN32
#define perror(x) printf("%s : code d'erreur : %d\n", (x), WSAGetLastError())
#define close closesocket
#define socklen_t int
#endif

/* Variables cachees */

/* le socket d'ecoute */
int socketEcoute;
/* longueur de l'adresse */
socklen_t longeurAdr;

typedef struct {
	/* le socket de service */
	int socketService;
	/* le tampon de reception */
	char tamponClient[LONGUEUR_TAMPON];
	int debutTampon;
	int finTampon;
} ClientDesc;

/* Initialisation.
 * Creation du serveur.
 */
int Initialisation() {
	return InitialisationAvecService("13214");
}

/* Initialisation.
 * Creation du serveur en prcisant le service ou numro de port.
 * renvoie 1 si a c'est bien pass 0 sinon
 */
int InitialisationAvecService(char *service) {
	int n;
	const int on = 1;
	struct addrinfo	hints, *res, *ressave;

	#ifdef WIN32
	WSADATA	wsaData;
	if (WSAStartup(0x202,&wsaData) == SOCKET_ERROR)
	{
		printf("WSAStartup() n'a pas fonctionne, erreur : %d\n", WSAGetLastError()) ;
		WSACleanup();
		exit(1);
	}
	memset(&hints, 0, sizeof(struct addrinfo));
    #else
	bzero(&hints, sizeof(struct addrinfo));
	#endif

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(NULL, service, &hints, &res)) != 0)  {
     		printf("Initialisation, erreur de getaddrinfo : %s", gai_strerror(n));
     		return 0;
	}
	ressave = res;

	do {
		socketEcoute = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socketEcoute < 0)
			continue;		/* error, try next one */

		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
#ifdef BSD
		setsockopt(socketEcoute, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif
		if (bind(socketEcoute, res->ai_addr, res->ai_addrlen) == 0)
			break;			/* success */

		close(socketEcoute);	/* bind error, close and try next one */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {
     		perror("Initialisation, erreur de bind.");
     		return 0;
	}

	/* conserve la longueur de l'addresse */
	longeurAdr = res->ai_addrlen;

	freeaddrinfo(ressave);
	/* attends au max 4 clients */
	listen(socketEcoute, 4);
	printf("Creation du serveur reussie.\n");

	return 1;
}

/* Attends qu'un client se connecte.
 */
int AttenteClient(Client *cl) {
	struct sockaddr *clientAddr;
	char machine[NI_MAXHOST];
	ClientDesc *client;

	client = (ClientDesc*) malloc(sizeof(ClientDesc));
	clientAddr = (struct sockaddr*) malloc(longeurAdr);
	client->socketService = accept(socketEcoute, clientAddr, &longeurAdr);
	if (client->socketService == -1) {
		perror("AttenteClient, erreur de accept.");
		free(clientAddr);
		free(client);
		return 0;
	}
	if(getnameinfo(clientAddr, longeurAdr, machine, NI_MAXHOST, NULL, 0, 0) == 0) {
		printf("Client sur la machine d'adresse %s connecte.\n", machine);
	} else {
		printf("Client anonyme connecte.\n");
	}
	free(clientAddr);
	/*
	 * Reinit buffer
	 */
	client->debutTampon = 0;
	client->finTampon = 0;
	*cl = (void*) client;
	return 1;
}

/* Recoit un message envoye par le serveur.
 */
char *Reception(Client cl) {
	char message[LONGUEUR_TAMPON];
	int index = 0;
	int fini = FALSE;
	int retour = 0;
	ClientDesc *client = (ClientDesc*)cl;
	while(!fini) {
		/* on cherche dans le tampon courant */
		while((client->finTampon > client->debutTampon) &&
			(client->tamponClient[client->debutTampon]!='\n')) {
			message[index++] = client->tamponClient[client->debutTampon++];
		}
		/* on a trouve si on a récupéré quelque chose et que le prochain caractère est \n */
		if ((index > 0) && (client->tamponClient[client->debutTampon]=='\n')) {
			message[index++] = '\n';
			message[index] = '\0';
			client->debutTampon++;
			fini = TRUE;
			/* printf("Etat tampon : debut = %d ; fin = %d ; prochain caractere : %d \n", client->debutTampon, client->finTampon, 
				client->tamponClient[client->debutTampon]); */
#ifdef WIN32
			return _strdup(message);
#else
			return strdup(message);
#endif
		} else {
			/* il faut en lire plus */
			/* printf("Lecture socket\n"); */
			client->debutTampon = 0;
			retour = recv(client->socketService, client->tamponClient, LONGUEUR_TAMPON, 0);
			/* printf("Reception de %d caracteres\n", retour); */
			if (retour < 0) {
				perror("Reception, erreur de recv.");
				return NULL;
			} else if(retour == 0) {
				fprintf(stderr, "Reception, le client a ferme la connexion.\n");
				return NULL;
			} else {
				/*
				 * on a recu "retour" octets
				 */
				client->finTampon = retour;
			}
		}
	}
	return NULL;
}

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 */
int Emission(char *message, Client cl) {
	int taille;
	ClientDesc *client = (ClientDesc*)cl;
	if(strstr(message, "\n") == NULL) {
		fprintf(stderr, "Emission, Le message n'est pas termine par \\n.\n");
		return 0;
	}
	taille = strlen(message);
	if (send(client->socketService, message, taille,0) == -1) {
        perror("Emission, probleme lors du send.");
        return 0;
	}
	printf("Emission de %d caracteres.\n", taille+1);
	return 1;
}


/* Recoit des donnees envoyees par le client.
 */
int ReceptionBinaire(char *donnees, size_t tailleMax, Client cl) {
	size_t dejaRecu = 0;
	int retour = 0;
	ClientDesc *client = (ClientDesc*)cl;
	/* on commence par recopier tout ce qui reste dans le tampon
	 */
	while((client->finTampon > client->debutTampon) && (dejaRecu < tailleMax)) {
		donnees[dejaRecu] = client->tamponClient[client->debutTampon];
		dejaRecu++;
		client->debutTampon++;
	}
	/* si on n'est pas arrive au max
	 * on essaie de recevoir plus de donnees
	 */
	if(dejaRecu < tailleMax) {
		retour = recv(client->socketService, donnees + dejaRecu, tailleMax - dejaRecu, 0);
		if(retour < 0) {
			perror("ReceptionBinaire, erreur de recv.");
			return -1;
		} else if(retour == 0) {
			fprintf(stderr, "ReceptionBinaire, le client a ferme la connexion.\n");
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

/* Envoie des donnes au client en prcisant leur taille.
 */
int EmissionBinaire(char *donnees, size_t taille, Client cl) {
	int retour = 0;
	ClientDesc *client = (ClientDesc*)cl;
	retour = send(client->socketService, donnees, taille, 0);
	if(retour == -1) {
		perror("Emission, probleme lors du send.");
		return -1;
	} else {
		return retour;
	}
}



/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client cl) {
	ClientDesc *client = (ClientDesc*)cl;
	close(client->socketService);
	free(client);
}

/* Arrete le serveur.
 */
void Terminaison() {
	close(socketEcoute);
}


/*
 * Fonctions creation des réponse
 * Type : COMMANDE, 
 * typeRep : Positive(0) ou négative(1)
 * Renvoi 0 si OK, 1 sinon
 */ 

int envoiRep(char* type, char* login, int typeRep, Client cl ){
  char* reponse = NULL;
  char* reponse_temp = NULL;
  char* login_temp = NULL;

  reponse = malloc((size_t)50);
  reponse_temp = malloc((size_t)50);
  login_temp = malloc(strlen(login));
  login_temp = strncpy(login_temp, login, strlen(login));

  if(typeRep == 0){
    reponse_temp = strncat(type, "_ACK ", strlen("_ACK "));
  }else{
    reponse_temp = strncat(type, "_ERR ", strlen("_ERR "));
  }

    reponse = strncat(reponse_temp, login_temp, strlen(login_temp));
    reponse = strcat(reponse, "\n");
  
    if(Emission(reponse, cl)==0){
      perror("\n Erreur lors de l'emission de la réponse");
      /* if (reponse !=NULL){ */
      /* 	free(reponse); */
      /* 	reponse = NULL;} */
      /* if (reponse_temp !=NULL){ */
      /* 	free(reponse_temp); */
      /* 	reponse_temp = NULL;} */
      /* if(login_temp != NULL){ */
      /* 	free(login_temp); */
      /* 	login_temp = NULL;} */
      return 1;
    }else {
    /* if (reponse !=NULL){ */
    /*   free(reponse); */
    /*   reponse = NULL;} */
    /* if (reponse_temp !=NULL){ */
    /*   free(reponse_temp); */
    /*   reponse_temp = NULL;} */
    /* if(login_temp != NULL){ */
    /*   free(login_temp); */
    /*   login_temp = NULL;} */

    return 0;
    }
 
   return 0;
  
}
  


/* 
 * Fonction afin de décoder les requêtes et les envoyer vers la fonction correspondante
 * renvoie 1 si connexion réussi
 */
int decodeRequete(char* message, char* pathDB, Client cl){
  char* login;
  char* extr;// utiliser pour l'extraction
  char* password;
  char* requete;
  int resReq = 0;


  // Utilisé pour extraire une chaine grâce à un délimiteur ici espace
  // Le premier prend tout ce qui est à droite de " "
  requete = strtok(message," ");  
  login = strtok(NULL, " "); // Le second tout ce qui est à droite du second delimiteur
  password = strtok(NULL, " "); // etc..


  if( strcmp(requete, "CON_USER") == 0){
    if(verificationLog(login, password, pathDB) == 0){
      // Authentification réussie
      envoiRep(requete, login, 0,cl);
      resReq = 1;
    }
    else{
      // Authentification échouée
      envoiRep(requete, login, 1, cl);
      resReq = 10;
    }
     
    return resReq;
  }

  /* free(password); */
  /* free(requete); */
  /* free(login); */



  // Ici toutes les requêtes, faudrait ce mettre d'accord sur lesquelles on fait

  
  
  return 0;



}


/*
 * Fonction de verification d'information de connexion d'un utilisateur sur un client
 * A intégrer dans une plus grande fonction "decodeReq"
 */

/*
 * Authentification réussie , renvoie 0 sinon 1
 */
int verificationLog(char* login, char* password, char* databasePath){
   char* ligneLog=NULL; // Lu dans le fichier => Utile pour faire les tests
   char* reponse=NULL;
   FILE* dataLog;
   char* loginLu; 
   char* mdpLu;

  
  // Allocation de la mémoire
  ligneLog = malloc((size_t)150);
 

  //Ouverture du fichier "database"
  dataLog = fopen(databasePath,"r");
  
  if(dataLog!= NULL){
    printf("\n\nOuverture du fichier\n"); //=> OK

    //On parcourt le fichier avec des strncmp
    do{
      fgets(ligneLog, 150 , dataLog);
      
      // On récupere le login lu dans le fichier
      loginLu = strtok(ligneLog, " ");
      
      // Test du login
      if (strcmp(login, loginLu)==0){
	mdpLu = strtok(NULL, " "); // On recupere le mdp Lu dans le fichier

	if(strcmp(password, mdpLu)==0){ // On teste, si Oui on retourne 0 => Authentification réussie
	  printf("\n\t Authentification réussie : %s\n ", login);
	  fclose(dataLog);
	  
	  return 0;
	}
      }
      
    }while(feof(dataLog)== 0);


  }else{
    perror("\nErreur lors de l'ouverture du fichier");
    // Fermeture du fichier et liberation de la mémoire lors d'une erreur d'ouverture
  }

    printf("\n\t Echec de l'Authentification \n");  


  fclose(dataLog);
  free(ligneLog);
  
  
  return 1; // Authentification échouée
}

//** Récupérer et modifier les fonctions découperRequêtes et executerRequetes

void *thread(void *arg) {
	/* On récupère le paramètre. Ici de type void* */
	Client cl = arg;
	char* message = NULL;
	char* path = "/home/tibo/DropBoxProject/Sources/log.txt";
	int resReq = 0;

		do {
		if(message != NULL) {
			free(message);
		}


	       	message = Reception(cl);

		// On décode la requête envoyé par le client
		
		switch(decodeRequete(message, path, cl)){
		case 1 : 
		  // Ici l'utilisateur est correctement connecté
		  // Il faudra je pense faire tout le traitement afin de différencier la partie connexion du reste 
		  // du programme. 
		  message = Reception(cl);
		  printf("\n Message reçu : %s ", message);
		  break;
		case 10 :
		  printf("\n\t Nouvelle tentative de connexion");
		  break;
		default :  
		  break;
		}
		  
	 
		
		if(message != NULL) {
		  // printf("J'ai recu: %s\n Je vais émettre maitenant", message);
		  // Decoupage et execution de la requete */
		  //  Emission("BienRecuCl\n",cl) ;
		  // EmissionBinaire(char *donnees, size_t taille, cl)
		       		}
	} while(message != NULL);

	TerminaisonClient(cl);
	pthread_exit(NULL);
}





