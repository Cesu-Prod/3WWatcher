=======================================
Worldwide Weather Watcher Documentation
=======================================

**Version** : 1.0

**Date** : 01/11/2024

**Auteurs** : Clément Richard, Nathan Polette, Noé Poirier


Description Générale
====================

Le Worldwide Weather Watcher est un dispositif de station-météo qui, via différents capteurs, permet de mesurer la pression atmosphérique, la température de l'air, l'hygrométrie, la luminosité ainsi que de connaître sa position GPS.

Ces données pourront être enregistrés sur une carte SD afin d'être utilisées pour prévoir des catastrophes naturelles.

Ce système embarqué comporte deux boutons qui permettront à l'utilisateur de naviguer entre quatres différents modes de fonctionnement :

   * standard
   * configuration
   * maintenance
   * économique

Ces différents modes permettront à l'utilisateur d'enregistrer des données avec une consommation énergétique différente en fonction de ses besoins, de configurer le système ou d'en effectuer la maintenance.

Enfin, ce dispositif comporte une gestion d'erreurs grâce à une LED RGB.


.. contents:: Sommaire
   :depth: 2
   :backlinks: none


Architecture Matérielle
=======================

**Microcontrôleur** : AVR ATmega328 intégré à la carte Arduino

**Composants** :
   * Lecteur de carte SD
   * Horloge RTC
   * LED RGB
   * 2 Boutons poussoirs

**Capteurs** :
   * Pression atmosphérique
   * Température de l'air
   * Hygrométrie
   * GPS
   * Luminosité


Description des Composants
--------------------------

**Microcontrôleur**
   * Informations Générales
      * Référence : ATMEGA328P-PU
      * Fabricant : Atmel
      * Fabrication : ATM
      * Type de Boîtier : PDIP-28 (Plastic Dual In-line Package)
   * Performances
      * Cadence d'Horloge : 20 MHz
      * Série : AVR® ATmega
      * Taille du Coeur : 8 bits
      * Processeur : Multi-coeur AVR
   * Oscillation et Alimentation
      * Type d'Oscillateur : Interne
      * Tension d'Alimentation Max : 5.5 V
      * Tension d'Alimentation Min : 1.8 V
   * Température de Fonctionnement
      * Plage de Température : -40 °C à +85 °C
   * Mémoire
      * Type de Mémoire Programme : FLASH
      * Taille de la Mémoire Programme : 32 KB (16 K x 16)
      * Taille EEPROM : 1 K x 8
      * Taille de la RAM : 2 K x 8
   * Périphériques et Entrées/Sorties
      * Nombre d'I/O : 23
      * Périphériques Intégrés :
         * Brown-out Detect/Reset
         * Power-On Reset (POR)
         * Modulation de Largeur d'Impulsion (PWM)
         * Watchdog Timer (WDT)
   * Convertisseur
      * Convertisseur de Données (A/D) : 6 canaux, 10 bits
   * Interfaces
      * Interface :
         * I2C
         * SPI
         * UART/USART

**Composants** :
   * Lecteur de carte SD
      * .. warning:: Référence : 
      * Protocole de communication : SPI
      * Fonction : Sauvegarde des données des capteurs.
   * Horloge RTC
      * .. warning:: Référence : 
      * Protocole de communication : I2C
      * Fonction : Fournir au système la date et l'heure du jour.
   * LED RGB
      * Référence : 
      * Protocole de communication : 2-wire
      * Fonction : Communiquer l'état du système.
   * Boutons poussoirs
      * Référence : 
      * Protocole de communication : Numérique
      * Fonction : Gérer l'interaction avec le système.

**Capteurs** :
   * BME280
      * Fonctions :
         * Pression atmosphérique
            * Plage de mesure : 300 à 1100 hPa avec une précision de ±1 hPa
         * Température de l'air
            * Plage de mesure : -40 °C à +85 °C avec une précision de ±1 °C
         * Hygrométrie
            * Plage de mesure : 0 à 100 % HR avec une précision de ±3 % HR
      * Alimentation : 1.71 V à 3.6 V
      * Protocole de communication : I2C ou SPI
   * GPS
      * Référence : AIR530
      * Alimentation : 3.3 V à 5 V
      * Temps de Démarrage
         * Démarrage à Chaud : 4 secondes
         * Démarrage à Froid : 30 secondes
      * Protocole de communication : UART
   * Luminosité
      * Référence : Grove - Light Sensor
      * Plage de mesure : 0 à 1023 lux
      * Alimentation : 3.3 V à 5 V
      * Protocole de communication : Analogique


Fonctionnement
==============

.. note:: Fonctionnement


Architecture Logicielle
=======================

.. note:: Architecture Logicielle


Code Source
=======================

.. note:: Code Source


Gestion des erreurs
===================

.. note:: Gestion des erreurs