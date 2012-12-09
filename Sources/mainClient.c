#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"



int main() {
	char *message;
	char choix[1024]; // Pourquoi un tableau?
	int fini = 0;
	char nomFichier[50]; 
		     
	Initialisation("127.0.0.1");
	connexion();
	
	message = Reception();
	printf("J'ai recu: %s\n", message);
	
	if(decodeRep(message) == 0){ // Si réponse connexion positive
	  
	  while(!fini){
	    afficheMenu();
	    
	    if(fgets(choix, 1024, stdin) != NULL){

	      switch(choix[0]){
	      case '0':
		fini = 1;
		break;
	      case '1':
		printf("Nom du fichier ?\n");
		scanf("%s",nomFichier);
		gererEmission(nomFichier);
		message = Reception();
		printf("J'ai recu: %s\n", message);
		break;
	      case '6':
		addUtilisateur();
		message = Reception();
		// decodeRep => traitement ok ou non 
		printf("J'ai recu: %s\n", message);
	      default:
		printf("Veuillez choisir une option correcte.\n");
	      }

	    }else {
	      printf("Veuillez choisir une option correcte.\n");
	    }
	  }
	}else{
	  printf("\nErreur lors de l'authentification, veuillez recommencer\n");
	  fini = 1;
	}
       
	Terminaison();

	return 0;
}
