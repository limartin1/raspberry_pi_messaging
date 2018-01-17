#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "tcp.h"
#include "ctrl_leds.h"


int main()
{
	int sockfd;
	char line[BUFSIZ]; 					/* Chaine de caractère stockant ce qui est reçu sur le socket entre ce daemon et le dameon principal */
	char must_blink_verte = 0; 		/* Variable stockant l'état dans lequel doit être le clignotement de la led */

	fprintf(stderr, "[dbg] boot ctrl_leds\n");
	fflush(stderr);
	
	sockfd = tcp_connect("127.0.0.1", "7000");  		/* Connexion au socket (en local puisque le socket est entre deux daemons sur la pi */
	if(sockfd == -1) {
		perror("tcp_connect");
		return 1;
	}

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	init(); 				/* Appel à la fonction d'initialisation */

	fprintf(stderr, "[info] initialization done\n");

	while (1)
	{
		*line = 0;		/* Vidage de la chaine de caractère stockant ce qui est reçu sur le socket */
		readLine(line, BUFSIZ, sockfd); 		/* Appel à la fonction de lecture sur l'uart */

		if(*line) {		/* Si il y a quelque chose de lu sur le socket */
			fprintf(stderr, "[info] line: %s\n", line);

			if(strcmp(line, "shut") == 0)						/* Si la commande est shut, c'est que le daemon principal n'a pas pu se lancer totalement, on quitte donc ce daemon et on le relande */
			{
				exit(-1);
			}

			if(strcmp(line, "blinkv") == 0) {					/* Si la commande reçu est blinkv, on change l'état dans lequel doit être la led verte à clignotant */
				must_blink_verte = 1;
			} else if(strcmp(line, "!blinkv") == 0) {		/* Si la commande reçu est !blinkv, on change l'état dans lequel doit être la led verte à non clignotant */
				must_blink_verte = 0;
			}

			if(strcmp(line, "lightj") == 0) {					/* Si la commande reçu est lightj, allumage de la led jaune */
				setValue(LEDJ, "1");
			} else if(strcmp(line, "!lightj") == 0) {		/* Si la commande reçu est !lightj, extinction de la led jaune */
				setValue(LEDJ, "0");
			}

			if(strcmp(line, "lightr") == 0) {					/* Si la commande reçu est lightr, allumage de la led rouge */
				setValue(LEDR, "1");
			} else if(strcmp(line, "!lightr") == 0) {		/* Si la commande reçu est !lightr, extinction de la led rouge */
				setValue(LEDR, "0");
			}
		}

		if(must_blink_verte) {									/* Si l'état de la led verte doit être clignotante, on fait clignoter en allumant 1 seconde et en éteignant une seconde */
				setValue(LEDV, "1");
				sleep(1);
				setValue(LEDV, "0");
				sleep(1);
			}
	}

	uninit();
}

void readLine(char *line, int n, int sockfd)			/* Fonction qui lit ce qui est sur le socket entre les deux daemons */
{
	char buffer = 0;
	int l;
	int i = 0;

	while (-1 != (l = read(sockfd, &buffer, 1)))		/* Tant qu'on détecte quelque chose sur le socket, on lit le socket (lettre à lettre) */
	{
		if (l > 0) 
		{
			while(buffer != '\n' && i < n)
			{
				fprintf(stderr, "[info] buffer: '%c'\n", buffer);
				line[i] = buffer;
				i++;
				usleep(1000);											/* Pause nécessaire pour ne pas que les lettres soient répétées */
				read(sockfd, &buffer, 1);	
			}
			line[i] = 0;
			return;
		}
		else
		{
			i = 0;
			sleep(1);
		}
	}
}

/* Permet de mettre la valeur demandée (0 ou 1) sur le GPIO*/
void setValue(char *led, char *v)
{
	char path[BUFSIZ];
	int fd;
	sprintf(path, "/sys/class/gpio/gpio%s/value", led);
	fd = open(path, O_WRONLY);

	write(fd, v, strlen(v));
}

/* Permet de configurer le GPIO en entrée ou en sortie*/
void setDirection(char *led)
{
	char path[BUFSIZ];
	int dir;

	sprintf(path, "/sys/class/gpio/gpio%s/direction", led);
	dir = open(path, O_WRONLY);
	if (dir == -1)
	{
		perror("Error Direction");
		exit(errno);
	}

	write(dir, "out", 3);
	close(dir);
}

/* Fonction d'initialisation des fichiers gérant les GPIO*/
void init()
{
	int exp = open("/sys/class/gpio/export",O_WRONLY);
	if(exp == -1)
	{
		perror("Error Export");
		exit(errno);
	}
	
	write(exp, LEDJ, strlen(LEDJ));
	write(exp, LEDV, strlen(LEDV));
	write(exp, LEDR, strlen(LEDR));

	setDirection(LEDJ);
	setDirection(LEDV);
	setDirection(LEDR);
	
	close(exp);
}

/* Permet de libérer l'utilisation des GPIO*/
void uninit()
{
	int unexp = open("/sys/class/gpio/unexport",O_WRONLY);
	if (unexp != -1)
	{
		write(unexp, LEDJ, strlen(LEDJ));
		write(unexp, LEDV, strlen(LEDV));
		write(unexp, LEDR, strlen(LEDR));
		close(unexp);
	}
	else
		perror("[uninit] open()");
}
