#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <arpa/inet.h>

#include "uart.h"
#include "tcp.h"

#include "main_daemon.h"


/* Variables globales */
int uart = 0; /* uart file descriptor */
int sock = 0; /* socket file descriptor */
int serveur = 0; /* server file descriptor */



/* Fonction principale lancée par le service main_daemon au démarrage de la pi */
/* Lance l'initialisation, demande un login et boucle ensuite en alternant une lecture sur le serveur de mail et une lecture sur la connexion uart */
int main()
{
	fprintf(stderr, "[dbg] boot main_daemon\n"); 
	fflush(stderr);

	init(); 								/* Fonction d'initialisation des connexions (uart, serveur et socket avec le daemon controleur de leds */	
	login(); 							/* Fonction d'authentification au serveur */
	while(1)
	{		
		lectureServeur(); 		/* Fonction qui lit ce qui est envoyé à la pi par le serveur */	
		lectureUart();				/* Fonction qui lit ce qui est envoyé à la pi par l'uart */	
	}
}


/* Fonction d'initialisation des connexions (uart, serveur et socket avec le daemon controleur de leds */	
void init()
{
	int sockfd;
	int lg;
	int option = 1;
	uart = uart_open(UART);
	if (uart == -1)
	{
		perror("uart_open()");
		exit(errno);
	}


	/* Création socket entre les deux services (celui-ci et le gestionnaire de leds) */
	sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof option);
	
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7000);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(sockfd, (const struct sockaddr *) &addr, (socklen_t) sizeof(addr));

	struct sockaddr_in newaddr;
	lg = sizeof(newaddr);
	/* Fin de la création du socket */
	
		
	/* Etablissement d'une connexion entre les deux services */	
	if(listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(errno);
	}

	printf("En attente de connexion au daemon gestionnaire de led verte\n");
	while( (sock = accept(sockfd, (struct sockaddr *) &newaddr, (socklen_t *) &lg)) == -1)
	{
		sleep(1);
	}

	printf("Le daemon gestionnaire de led verte est connecté\n");
	/* Fin de l'établissement de la connexion */


	/*Etablissement de la connexion au serveur TCP*/
	serveur = tcp_connect(ADDRESS_SERV, PORT_SERV);
	fprintf(stderr, "[dbg] serveur: %d\n", serveur);
	if(serveur == -1) 
	{
		perror("Connection serveur");
		char quit[] = "shut\n";
		int err = write(sock, quit, strlen(quit));
		fprintf(stderr, "[dbg] written: %d\n", err);
		if (err == -1)
		{
			perror("exit ne fonctionne pas !");
		}
		exit(errno);
	}
	/* Fin de l'établissement de la connexion */

	
	/* Création de 3 fichiers vierges : un fichier stockant les messages non lus, un pour les messages lus et un pour les messages envoyés */
	{
		int clr_un_fd = creat(MSG_UNREAD, 0660);
		if(clr_un_fd != -1) close(clr_un_fd);
	}
	{
		int clr_al_fd = creat(MSG_READ, 0660);
		if(clr_al_fd != -1) close(clr_al_fd);
	}
	{
		int clr_st_fd = creat(MSG_SENT, 0660);
		if(clr_st_fd != -1) close(clr_st_fd);
	}
}


/* Fonction d'authentification au serveur */
void login()
{
	int i = 0;
	int log = 0;										/* Variable servant à savoir si on est loggé sur le serveur */
	
	char buffer;
	char tmp_nick[BUFSIZ] = "";			/* Chaine de caractères stockant ce que l'utilisateur écrit sur l'uart (le pseudo dans le cas présent) */
	char ask_cmd[BUFSIZ]; 				/* Chaine de caractères stockant un message à écrire sur l'uart pour être lu par l'utilisateur */
	char ack[6] = "";								/* Chaine de caractères stockant l'acquittement du serveur */

	strcpy(ask_cmd, "\nEntrez votre pseudo : (Forme : \"nick PSEUDO\")\n");

	while(log == 0)								/* On boucle tant que l'on est pas loggé */
	{
		while(strncmp(tmp_nick, "nick ", 5)) 	/* On boucle 	tant que l'utilisateur n'a pas rentré une commande */
		{
			i = 0;
			int test2 = write(uart, ask_cmd, strlen(ask_cmd));	/* Ecriture du message de demande de pseudo sur l'uart*/
			if (test2 == -1)
			{
				perror("ecriture uart demande de pseudo"); 
			}			

			/* Lecture de la commande sur l'uart*/
			buffer = 0;
			while(buffer != '\n')				
			{
				test2 = read(uart, &buffer, 1);
				if (test2 == -1)
				{
					perror("lectrue uart pseudo");
				}

				tmp_nick[i] = buffer;
				i++;
			} 
			tmp_nick[i] = 0;
			/* Fin de la lecture*/
		}
		

		/* Ecriture de la commande nick sur le serveur */
		int test1 = write(serveur, tmp_nick, strlen(tmp_nick));
		if (test1 == -1)
		{
			perror("connection serveur nick");
		}

		/* Lecture de l'acquittement du serveur */
		test1 = read(serveur, ack, 6);
		if (test1 == -1)
		{
			perror("lecture ack serveur nick");
		}


		/* Vérification du bon acquittement (dans ce cas on est loggé) */
		if(strncmp(ack, "ack 0", 5) == 0)
		{
			log = 1;
		}
		else /* Traitement des cas d'erreurs en cas de mauvais acquittement */
		{
			if(strncmp(ack, "ack 2", 5) == 0)
			{
				strcpy(ask_cmd, "Attention, il manque un argument dans la commande\n");
			}
			else if(strncmp(ack, "ack 3", 5) == 0)
			{
				strcpy(ask_cmd, "Attention, le pseudo est deja utilise\n");
			}
			else if(strncmp(ack, "ack 4", 5) == 0)
			{
				strcpy(ask_cmd, "Attention, le pseudo ne respecte pas le format attendu\n");
			}
			else
			{
				strcpy(ask_cmd, "Il y a un probleme inconnu avec le pseudo. Veuillez reessayer\n");
			}
			write(uart, ask_cmd, strlen(ask_cmd));
			strcpy(ask_cmd, "Entrez votre pseudo : (Forme : \"nick PSEUDO\")\n");
			strcpy(tmp_nick, "");
		}
	}
}


/* Fonction qui lit ce qui est envoyé à la pi par le serveur */	
void lectureServeur()
{
	fcntl(serveur, F_SETFL, O_NONBLOCK);

	FILE * fic_unmess;
	fic_unmess = fopen(MSG_UNREAD, "a+");

	if(fic_unmess == NULL)
	{
		perror("Erreur fic_unmess");
		exit(errno);
	}
	
	char buffer;
	char buffer_mot[BUFSIZ] = "";  		/* Chaine de caractères stockant ce qui est reçu depuis le serveur */
	char mess_rec[BUFSIZ] = "";			/* Chaine de caractères stockant le message reçu (dans le cas d'un recv) */
	
	int lg;
	int i = 0;

	while (-1 != (lg = read(serveur, &buffer, 1))) /* On boucle tant qu'on lit quelque chose depuis le serveur (lettre à lettre) */
	{
		if (lg > 0) 
		{
			/* Lecture de la totalité de ce qui a été reçu depuis le serveur */
			while(buffer != '\n')
			{
				buffer_mot[i] = buffer;
				i++;
				usleep(1000); 									/* Pour éviter qu'une même lettre soit lue plusieurs fois */
				read(serveur, &buffer, 1);	
			}
			buffer_mot[i] = '\n';
			buffer_mot[i+1] = 0;
			/* Fin de la lecture */
		}
		else
		{
			i = 0;
			sleep(1);
		}
	}

	if(strncmp(buffer_mot, "recv ", 5) == 0)  /* Si ce qui est reçu depuis le serveur est un message (recv) */
	{
		fputs(buffer_mot+5, fic_unmess);      /* On ajoute ce message à la fin du fichier de messages non lus */
		{
			/* Envoi de la commande de clignotement de la led verte au daemon gestionnaire de leds */
			char blinkv[] = "blinkv\n";	
			write(sock, blinkv, strlen(blinkv));
		}
	} 

	else if(strncmp(buffer_mot, "ack ", 4) == 0) /* Si ce qui est reçu depuis le serveur est un acquitement  */
	{
		/* Envoi de la commande d'allumage de la led jaune au daemon gestionnaire de leds */
		char nolightj[] = "!lightj\n"; 	
		write(sock, nolightj, strlen(nolightj));
		usleep(50000); /*Attente necessaire à la prise de la compte de la commande par le daemon gestionnaire de leds */

		/* Pour récupérer le message dans le cas d'un send all (car ce qui est reçu depuis le serveur est "ack X recv PSEUDO MESSAGE" sur la même ligne) */
		int lg = strlen(buffer_mot);
		if (lg > 4)
		{
			/* Lecture du message reçu depuis le serveur */
			for(i=5; i<lg; i++)
			{
				mess_rec[i-5] = buffer_mot[i];
			}
			mess_rec[i] = '\n';
			mess_rec[i+1] = 0;
			/* Fin de la lecture */
		}
		
		/* Gestion d'erreur si le ack ne vaut pas 0 */
		if (strncmp(buffer_mot, "ack 0", 5)) 
		{
			/* Envoi de la commande d'allumage de la led rouge au daemon gestionnaire de leds */
			char lightr[] = "lightr\n";
			write(sock, lightr, strlen(lightr));
			usleep(50000); /*Attente necessaire à la prise de la compte de la commande par le daemon gestionnaire de leds */
		}
		

		if (strncmp(mess_rec, "recv ", 5) == 0) /* Si un message a été reçu depuis le serveur */
		{
			/* Stockage du message reçu dans le fichier des messages non lus */
			fputs(buffer_mot+10, fic_unmess); 
			{
				/* Envoi de la commande de clignotement de la led verte au daemon gestionnaire de leds */
				char blinkv[] = "blinkv\n";
				write(sock, blinkv, strlen(blinkv));
			}
		}
	}

	fclose(fic_unmess);
	fcntl(serveur, F_SETFL, ~O_NONBLOCK);
}


/* Fonction qui lit ce qui est envoyé à la pi par l'uart */	
void lectureUart()
{
	fcntl(uart, F_SETFL, O_NONBLOCK);
	char buffer;
	char buffer_mot[BUFSIZ];  /* Chaine de caractères stockant ce qui est reçu depuis l'uart (envoyé par l'utilisateur) */
	
	int lg;
	int i = 0;

	while (-1 != (lg = read(uart, &buffer, 1))) /* On boucle tant qu'on lit quelque chose depuis l'uart (lettre à lettre) */
	{
		if (lg > 0) 
		{
			while(buffer != '\n')
			{
				buffer_mot[i] = buffer;
				i++;
				usleep(1000);
				read(uart, &buffer, 1);	
			}
			buffer_mot[i] = '\n';
			buffer_mot[i+1] = 0;
		
			/* Lancement de l'analyse de ce qui est reçu depuis l'uart */	
			analyseCmd(buffer_mot); 
		}
		else
		{
			i = 0;
			sleep(1);
		}
	}

	fcntl(uart, F_SETFL, ~O_NONBLOCK);
}


/* Fonction qui analyse et exécute les commandes utilisateurs reçues sur l'uart */
void analyseCmd(char * cmd)
{
	char message[BUFSIZ];
	if (strncmp(cmd, "send", 4) == 0) /* Cas d'un send PSEUDO MESSAGE */
	{
		strcpy(message, "\nEnvoi d'un message\n");
		write(uart, message, strlen(message));

		/* Envoi des commandes concernant les leds au contrôleur de leds */
		char nolightr[] = "!lightr\n";
		write(sock, nolightr, strlen(nolightr));
		usleep(50000); /* Pour attendre que le daemon des leds reçoive bien la commande */
		char lightj[] = "lightj\n";
		write(sock, lightj, strlen(lightj));
		usleep(50000);
		write(serveur, cmd, strlen(cmd));

		/* Ouverture du fichier des messages envoyés */
		FILE * fsent  = fopen(MSG_SENT, "a+");
		if(fsent == NULL)
		{
			perror("Ouverture du fichier de messages envoyes impossible");
			exit(errno);
		}

		else
		{
			/* Récupération du message sans le "send " pour le stocker sur le fichier des messages envoyés */
			char msg[BUFSIZ] = "";
			int i = 0;
			int lg = strlen(cmd);
			for (i=5; i<lg; i++)
			{
				msg[i-5] = cmd[i];
			}
			msg[i] = '\n';
			msg[i+1] = 0;
			/* Fin de la récupération */
	
			fputs(msg, fsent);	/* Stockage du message sur le fichier de message non lus */
			fflush(fsent);	
			fclose(fsent);
		}
	}
	else if (strncmp(cmd, "read_un", 7) == 0)  /* Cas d'un read_un (lire les messages non lus) */
	{
		{
			/* Envoi de la commande pour arrêter le clignottement de la led verte */
			char buf[] = "!blinkv\n";
			write(sock, buf, strlen(buf));
		}

		strcpy(message, "\nLecture des messages non lus : \n");
		write(uart, message, strlen(message));

		/* Ouverture des fichiers de messages non lus et messages lus */
		FILE * fin  = fopen(MSG_UNREAD, "r");
		FILE * fout = fopen(MSG_READ, "a+");
		if(fin == NULL || fout == NULL)
		{
			perror("Ouverture des fichiers de messages impossible");
			exit(errno);
		}
		else
		{
			char msg[BUFSIZ];
			/* Récupération des messages non lus dans le fichier des messages non lus, écriture des messages sur l'uart pour lecture utilisateur et copie dans le fichier des messages lus */
			while(fgets(msg, BUFSIZ, fin) != NULL)
			{
				write(uart, msg, strlen(msg));
				fputs(msg, fout);
			}
			fclose(fin);
			fclose(fout);

			/* Création d'un nouveau fichier de messages non lus avec écrasement du précédent */
			{
				int clr_fd = creat(MSG_UNREAD, 0660);
				if(clr_fd != -1) close(clr_fd);
			}
		}
	}
	else if (strncmp(cmd, "read_al", 7) == 0) /* Cas de lecture des messages déjà lus */
	{
		strcpy(message, "\nLecture des messages deja lus : \n");
		write(uart, message, strlen(message));

		/* Ouverture du fichier des messages lus */
		FILE * fin  = fopen(MSG_READ, "r");
		if(fin == NULL)
		{
			perror("Ouverture du fichier de messages lus impossible");
			exit(errno);
		}
		else
		{
			char msg[BUFSIZ];
			/* Récupération des messages lus et écriture sur l'uart pour la lecture par l'utilisateur */
			while(fgets(msg, BUFSIZ, fin) != NULL)
			{
				write(uart, msg, strlen(msg));
			}
			fclose(fin);
		}
	}
	else if (strncmp(cmd, "read_st", 7) == 0) /* Cas de lecture des messages envoyés */
	{
		strcpy(message, "\nLecture des messages envoyes : \n");
		write(uart, message, strlen(message));

		/* Ouverture du fichier des messages envoyés */
		FILE * fsent  = fopen(MSG_SENT, "r");
		if(fsent == NULL)
		{
			perror("Ouverture du fichier de messages envoyes impossible");
			exit(errno);
		}
		else
		{
			char msg[BUFSIZ] = "";
			/* Récupération des messages envoyés et écriture sur l'uart pour la lecture par l'utilisateur */
			while(fgets(msg, BUFSIZ, fsent) != NULL)
			{
				write(uart, msg, strlen(msg));
			}
			fclose(fsent);
		}
	}
	else if (strncmp(cmd, "erase", 5) == 0) /* Cas pour effacer tous les messages */
	{
		strcpy(message, "\nEffacement des messages : \n");
		{
			/* Envoi de la commande pour arrêter le clignottement de la led verte */
			char buf[] = "!blinkv\n";
			write(sock, buf, strlen(buf));
		}
		write(uart, message, strlen(message));

		/* Création des nouveaux fichiers de messages avec écrasement des précédents */
		{
				int clr_fd = creat(MSG_UNREAD, 0660);
				if(clr_fd != -1) close(clr_fd);
		}
		{
				int clr_fd = creat(MSG_READ, 0660);
				if(clr_fd != -1) close(clr_fd);
		}
		{
				int clr_fd = creat(MSG_SENT, 0660);
				if(clr_fd != -1) close(clr_fd);
		}
	}
	else /* Cas où la commande reçu n'est pas comprise (ne fait pas partie de la liste) */
	{
		strcpy(message, "\nAttention, la commande entree n est pas valide\nListe des commandes valides :\nsend \"PSEUDO\" \"MESSAGE\"\nread_un\nread_al\nread_st\nerase\n");
		write(uart, message, strlen(message));
	}
}
