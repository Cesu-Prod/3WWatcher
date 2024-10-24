void setup() {
  Serial.begin(9600);
}

unsigned char buffer[64];         // Tableau pour stocker les données reçues sur le port série
int count = 0;                    // Compteur pour le tableau buffer

void loop() 
{
    // Si des données sont disponibles sur le port série matériel (venant du GPS sur les broches 0 et 1)
    if (Serial.available()) 
    {
        // Lire les données dans un tableau tant qu'elles sont disponibles et que la taille du tableau n'est pas dépassée
        while (Serial.available() && count < 64) 
        {
            buffer[count++] = Serial.read();  // Lecture des données et stockage dans le tableau buffer
        }
        Serial.write(buffer, count);          // Afficher le contenu du buffer dans le moniteur série
        clearBufferArray();                   // Appeler la fonction pour vider le buffer
        count = 0;                            // Réinitialiser le compteur à zéro
        delay(1000);                          // Réduire le délai pour améliorer la réactivité
    }
}

// Fonction pour vider le tableau buffer
void clearBufferArray() 
{
    for (int i = 0; i < count; i++) 
    {
        buffer[i] = 0;  // Effacer le contenu du tableau en le remplissant avec des zéros
    }
}