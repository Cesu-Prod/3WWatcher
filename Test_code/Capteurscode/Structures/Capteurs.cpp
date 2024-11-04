#include "Arduino.h"

// Fonction yield() déclarée comme faible pour éviter les erreurs de compilation
extern "C" void __attribute__((weak)) yield(void) {}

// Structure représentant un noeud de la liste
typedef struct Node {
    short int value; // Valeur du noeud
    Node *next; // Pointeur vers le noeud suivant
} Node;

// Structure représentant un capteur
typedef struct Sensor {
    uint8_t error;
    Node *head_list;

    Sensor () {
        Init_list();
    }
    
    void Init_list();
    void Update(short int value);
    short int Average();
} Sensor;

void Sensor::Init_list() {
    // Création du premier noeud de la liste
    Node *initial_node = new Node();
    initial_node->value = 0;
    initial_node->next = nullptr;
    head_list = initial_node;

    // Création des 3 noeuds suivants de la liste
    for (unsigned short int i = 1; i <= 3; i++) {
        Node *node = new Node;
        node->value = 0;
        node->next = head_list;
        head_list = node;
    }
}

void Sensor::Update(short int value) {
    // Création d'un nouveau noeud avec la valeur donnée
    Node *node = new Node();
    node->value = value;
    node->next = head_list;
    head_list = node;

    // Suppression du dernier noeud de la liste
    Node *current;
    current = head_list;
    while (current->next->next != nullptr) {
        current = current->next;
    }
    delete current->next;
    current->next = nullptr;
}

short int Sensor::Average() {
    // Calcul de la moyenne des valeurs du capteur
    Node *current;
    unsigned short int count = 0;
    short int total = 0;
    current = head_list;
    while (current != nullptr) {
        // Ignorer les valeurs nulles
        if (current->value != 0) {
            total += current->value;
            count++;
        }
        current = current->next;
    }
    // Vérification de la présence de valeurs
    if (count != 0) {
        // Calcul de la moyenne
        return total / count;
    } else {
        // Retourne 0 si aucune valeur n'est présente
        return 0;
    }
}

// Déclaration des capteurs
static Sensor ssr_lum; // Capteur de luminosité
static Sensor ssr_hum; // Capteur d'humidité
static Sensor ssr_tmp; // Capteur de température
static Sensor ssr_prs; // Capteur de pression




// Example of use.

void setup() {
}

void loop() {
    static int lum = 0;
    static int hum = 0;
    static int tmp = 0;
    static int prs = 0;

    lum = random(0, 100);
    hum = random(0, 100);
    tmp = random(0, 100);
    prs = random(0, 100);

    ssr_lum.Update(lum);
    ssr_hum.Update(hum);
    ssr_tmp.Update(tmp);
    ssr_prs.Update(prs);
}