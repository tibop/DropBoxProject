#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"



int main() {
	char *message;
	char choix[1024];
	int fini = 0;
	char nomFichier[50]; 
		     
	Initialisation("127.0.0.1");
	while(!fini) {
		
	  connexion();
	 
	
	  message = Reception();
	  printf("J'ai recu: %s\n", message);

	  /* Un souci avec les allocations mémoire je pense(méthode de test) :  */
	  /* 1. Connexion OK */
	  /* 2. Connexion NOK */
	  /* 3. Réessaie avec id valide => Ne se reconnecte pas. */


	  if(decodeRep(message) == 0){ // Si réponse connexion positive

	    afficheMenu();
	    
	    if(fgets(choix, 1024, stdin) != NULL) {
	      switch(choix[0]) {
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
	      default:
		printf("Veuillez choisir une option correcte.\n");
	      }
	    } else {
	      printf("Veuillez choisir une option correcte.\n");
	    }
	  }else{
	    printf("\nErreur lors de l'authentification, veuillez recommencer\n");
	    connexion();	    
	  }
	}

	Terminaison();

	return 0;
}
