#ifndef _GARDIEN_MAIN_DAEMON_
#define _GARDIEN_MAIN_DAEMON_

#define BACKLOG	1

#define	ADDRESS_SERV "172.16.32.27"
/*#define	ADDRESS_SERV "192.168.1.39" ADRESSE SERVEUR LILIAN*/
#define PORT_SERV "1337"

#define MSG_UNREAD "/home/samlil/notreprojet/messages/unread.txt"
#define MSG_READ "/home/samlil/notreprojet/messages/read.txt"
#define MSG_SENT "/home/samlil/notreprojet/messages/sent.txt"

#define UART "/dev/ttyAMA0"

void init();
void login();
void lectureServeur();
void lectureUart();
void analyseCmd(char *);

#endif
