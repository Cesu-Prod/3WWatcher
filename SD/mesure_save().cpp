#include <SPI.h>
#include <SdFat.h>

SdFat SD;

const int chip_slct = 10; // Pin CS pour la carte SD
File mes_file;
char file_name[12]; // Pour stocker le nom du fichier
int file_idx = 0; // Compteur pour le nom du fichier

void setup() {
    // Initialiser la carte SD
    if (!SD.begin(chip_slct)) {
        err_code = 6;
        return;
    }
}

void create_new_file() {
    // Générer un nom de fichier unique
    sprintf(file_name, "%02d%02d%02d_%01d.LOG", year % 100, month, day, file_idx);
    mes_file = SD.open(file_name, FILE_WRITE);
    if (!mes_file) {
        err_code = 6;
        return;
    }
}

void write_mes(String data) {
    // Vérifier le changement de date
    if (day != pre_day || month != pre_month || year != pre_year) {
        // Fermer le fichier actuel si ouvert
        if (mes_file) {
            mes_file.close();
        }

        // Réinitialiser l'index du fichier
        file_idx = 0;

        // Créer un nouveau fichier pour le nouveau jour
        create_new_file();

        // Mettre à jour le jour, le mois et l'année précédents
        pre_day = day;
        pre_month = month;
        pre_year = year;
    }

    if (mes_file.size() >= param[1].val) {
        // Fermer le fichier actuel
        mes_file.close();

        // Incrémenter l'index du fichier
        file_idx++;

        // Renommer le fichier
        char old_file_name[12];
        sprintf(old_file_name, "%02d%02d%02d_%01d.LOG", year % 100, month, day, file_idx);
        String new_file_name = String(file_name);
        SD.rename(old_file_name, new_file_name);
        
        // Créer un nouveau fichier
        create_new_file();
    }
    // Écrire les données dans le fichier
    mes_file.println(data);
}

void mesure_save() {
    write_mes(String(hour) + ":" + String(minute) + ":" + String(second) + " - " + 
               String(lat) + " " + String(lat_dir) + ", " + 
               String(lon) + " " + String(lon_dir) + ", " + 
               String(ssr_lum::Average()) + ", " + 
               String(ssr_hum::Average()) + ", " + 
               String(ssr_tmp::Average()) + ", " + 
               String(ssr_prs::Average()));
}