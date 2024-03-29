#ifndef __SERVEUR_H__
#define __SERVEUR_H__

typedef void *Client;

/* Initialisation.
 * Creation du serveur.
 * renvoie 1 si a c'est bien pass 0 sinon
 */
int Initialisation();

/* Initialisation.
 * Creation du serveur en prcisant le service ou numro de port.
 * renvoie 1 si a c'est bien pass 0 sinon
 */
int InitialisationAvecService(char *service);


/* Attends qu'un client se connecte.
 * renvoie 1 si a c'est bien pass 0 sinon
 */
int AttenteClient(Client *cl);

/* Recoit un message envoye par le client.
 * retourne le message ou NULL en cas d'erreur.
 * Note : il faut liberer la memoire apres traitement.
 */
char *Reception(Client cl);

/* Envoie un message au client.
 * Attention, le message doit etre termine par \n
 * renvoie 1 si a c'est bien pass 0 sinon
 */
int Emission(char *message, Client cl);

/* Recoit des donnees envoyees par le client.
 * renvoie le nombre d'octets reus, 0 si la connexion est ferme,
 * un nombre ngatif en cas d'erreur
 */
int ReceptionBinaire(char *donnees, size_t tailleMax, Client cl);

/* Envoie des donnes au client en prcisant leur taille.
 * renvoie le nombre d'octets envoys, 0 si la connexion est ferme,
 * un nombre ngatif en cas d'erreur
 */
int EmissionBinaire(char *donnees, size_t taille, Client cl);


/* Ferme la connexion avec le client.
 */
void TerminaisonClient(Client cl);

/* Arrete le serveur.
 */
void Terminaison();

void *thread(void *arg) ;

#endif