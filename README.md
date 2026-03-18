TP Systèmes à Microprocesseurs 16/32 bits - SAM D21
Ce repository contient l'intégralité du code développé dans le cadre du mini-projet de 4ème année à Polytech Nantes
. L'objectif final est de réaliser la commande asservie d'un moteur à courant continu via un signal PWM et un retour d'information par convertisseur ADC.

🛠 Environnement de Développement
Matériel : Carte d'évaluation SAMD21 Xplained Pro équipée du microcontrôleur ATSAMD21J18A (Cœur ARM Cortex-M0+).

Logiciel : Environnement Microchip Studio.

Chaîne de compilation : ARM GNU C Compiler et ARM GNU Assembler.

Librairies : Atmel Software Framework (ASF).

📂 Structure du Projet
Le développement a été réalisé de manière incrémentale, chaque étape validant une fonctionnalité critique du système:
 
Étape 1 : I/O et Power Manager
Configuration des ports d'entrée-sortie (PORT) et analyse de l'unité Power Manager (PM) pour l'activation des périphériques.

Étape 2 : Base de temps (Timer)
Configuration d'un Timer/Counter (TC) pour générer un signal carré de période 1 ms.

Étape 3 : Interruptions (NVIC)
Mise en œuvre du mécanisme d'interruption pour déclencher une routine de service (ISR) toutes les millisecondes.

Étape 4 : Génération de Gabarit (DAC)
Utilisation du convertisseur Numérique-Analogique (DAC) pour générer des rampes d'accélération et de décélération moteur (Repos, Accel, Vconst, Decel).

Étape 5 : Commande PWM et Modélisation
Génération d'un signal PWM à 10 kHz pour piloter le moteur et mise en place d'un filtre RRC (constante de temps de 10 ms) pour modéliser le comportement du moteur.

Étape 6 : Asservissement (ADC)
Lecture de la vitesse réelle via le convertisseur Analogique-Numérique (ADC) et implantation d'un algorithme d'asservissement en boucle fermée.

Étape 7 : Supervision (EIC)
Interaction utilisateur via le contrôleur d'interruptions externes (EIC) et simulation de messages LIN par appuis sur bouton-poussoir pour modifier la vitesse de consigne.

⚙️ Configuration Technique Clé
Pour chaque périphérique, la configuration suit le schéma suivant:

Clock Management : Activation de l'horloge système (GCLK) et démasquage de l'horloge de bus dans le Power Manager.
Pin Multiplexing : Redirection des broches physiques vers les fonctions périphériques via les registres PMUX.

Peripheral Init : Configuration des registres spécifiques (CTRLA, CTRLB, PER, CCx) en fonction des besoins (fréquence, résolution).

🚀 Utilisation
Cloner le repository.
Ouvrir la solution .atsln dans Microchip Studio
.
Connecter la carte SAMD21 via le port DEBUG USB
.
Compiler (Build solution) et flasher la cible (Start without debugging)
