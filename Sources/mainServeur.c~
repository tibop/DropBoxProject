#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "serveur.h"
#include <pthread.h>

pthread_mutex_t mutmut ; // Mutex de protection pour écriture 
pthread_cond_t cond_wr ;

/* 
 * Types de requêtes
 */
#define REQ_INCONNUE	0
#define AJOUT 		1




int main() {
	
      Client cl;
  
      Initialisation();

	
	while(1) {
		pthread_t th;
	  
		AttenteClient(&cl);
		/* On crée un thread en lui donnant la fonction à exécuter et les infos clients */
		pthread_create(&th, NULL, thread, cl);
		};
	
	return 0;
}
