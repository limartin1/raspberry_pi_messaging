# Messagerie via Raspberry Pi

### Description du projet
<p>Dans le cadre du cours Linux Embarqué en 2ème année ISIMA (filière Informatique des Systèmes Embarqués), nous avons du développer en binome un programme de gestion de messages à l'aide d'un Raspberry Pi.</p>

<p>Le Raspberry Pi sert d'intermédiaire entre un utilisateur utilisant le logiciel CuteCom et un serveur de messagerie interne à la salle de TP. L'utilisateur envoie des commandes via CuteCom en uart au Raspberry Pi et le programme s'occupe de traiter ces commandes. L'utilisateur doit pouvoir se logger sur le serveur, envoyer des messages à un ou tous les autres utilisateurs, lire les messages reçus non lus, lire les messages déjà lus ou encore lire les messages envoyés. Le Raspberry Pi gère 3 LEDS pour interragir avec l'utilisateur.</p>

### Gestion des LEDS
* Lorsqu'un mail est reçu, une LED verte clignote
* Lors de l'envoi d'un message, une LED jaune est allumée tant que le serveur n'acquitte pas
* Si l'acquitement du serveur est négatif (erreur), une LED rouge est allumée tant que l'utilisateur n'envoi pas un nouveau message

<p>Le programme est divisé en deux exécutables (main_daemon et ctrl_leds) qui communiquent entre eux par un socket TCP afin que la partie principale puisse gérer les LEDS sans bloquer le déroulement. Au démarrage du Raspberry Pi, les deux éxécutables sont lancés en tant que daemon. Le système est donc totalement autonome et ne nécessite pas de relier le Raspberry à un écran.</p>

<p>Il n'est par contre pas possible de le tester hors de la salle de TP car il faut que le programme se connecte au serveur de messagerie de la salle pour pouvoir fonctionner.</p>