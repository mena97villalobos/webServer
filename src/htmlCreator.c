//
// Created by bryan on 16/11/18.
//

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <regex.h>
#include <stdio.h>
#include <dirent.h>
#include "file.h"
#include "htmlCreator.h"

#define PATH_XML "../src/serverroot"
#define REGEX_NOMBRE "<nombre>(.*|\s*)<\/nombre>"
#define REGEX_DESC "<descripcion>(.*|\s*)<\/descripcion>"
#define REGEX_TAMANNO "<tamanno>(.*|\s*)<\/tamanno>"
#define REGEX_FECHA "<fecha>(.*|\s*)<\/fecha>"
#define REGEX_PREVIEW_PATH "<previewPath>(.*|\s*)<\/previewPath>"

#define HTML_HEADER_PATH "../src/serverroot/indexHeader.html"
#define HTML_FOOT_PATH "../src/serverroot/indexFoot.html"

void parseData(char* datos, int indexBloque){
    char* regexs[5] = {REGEX_NOMBRE, REGEX_DESC, REGEX_TAMANNO, REGEX_FECHA, REGEX_PREVIEW_PATH};
    int returns;
    struct xmlVideo* videoData = malloc(sizeof(struct xmlVideo));

    for(int i = 0; i < 5; i++){
        regex_t regex;
        regmatch_t pmatch[2];

        returns = regcomp(&regex, regexs[i], REG_ICASE|REG_EXTENDED);

        if (returns) {
            printf(stderr, "Could not compile regex\n");
            exit(1);
        }
        /* Execute regular expression */
        returns = regexec(&regex, datos, 2, pmatch, REG_EXTENDED);
        if (!returns) {

            int offset = (pmatch[1].rm_eo - pmatch[1].rm_so);

            switch (i){
                case 0:
                    videoData->nombre = calloc((offset + 1), sizeof(char));
                    memcpy(videoData->nombre, &datos[pmatch[1].rm_so], offset);
                    break;
                case 1:
                    videoData->descripcion = calloc((offset + 1), sizeof(char));
                    memcpy(videoData->descripcion, &datos[pmatch[1].rm_so], offset);
                    break;
                case 2:
                    videoData->tamanno = calloc((offset + 1), sizeof(char));
                    memcpy(videoData->tamanno, &datos[pmatch[1].rm_so], offset);
                    break;
                case 3:
                    videoData->fecha = calloc((offset + 1), sizeof(char));
                    memcpy(videoData->fecha, &datos[pmatch[1].rm_so], offset);
                    break;
                case 4:
                    videoData->previewPath = calloc((offset + 1), sizeof(char));
                    memcpy(videoData->previewPath, &datos[pmatch[1].rm_so], offset);
                default:
                    break;
            }
        }
        else if (returns == REG_NOMATCH) {
            puts("No match");
        }
        regfree(&regex);
    }

    FILE* f = fopen("../src/serverroot/index.html", "a");

    if(indexBloque == 0)
        fprintf(f, "%s\n", "<div class=\"container-fluid\"> <div class=\"media-container-row\">");

    fprintf(f, "%s%s%s", "<div class=\"card p-3 col-12 col-md-6 col-lg-2\">"
                     "<div class=\"card-wrapper\"><div class=\"card-img\"><a href=\"http://localhost:3940/", videoData->nombre,
                     ".mp4\"><img src=\"");
    fprintf(f, "%s", videoData->previewPath);
    fprintf(f, "%s", "\"></a></div><div class=\"card-box\"><h4 class=\"card-title pb-3 mbr-fonts-style display-7\">\n");
    fprintf(f, "%s", videoData->nombre);
    fprintf(f, "%s", "</h4><p class=\"mbr-text mbr-fonts-style display-7\">\n");
    fprintf(f, "Descripción: %s\nTamaño: %s\nFecha: %s\n", videoData->descripcion, videoData->tamanno, videoData->fecha);
    fprintf(f, "%s", "</p>\n</div>\n</div></div>");

    if(indexBloque == 5)
        fprintf(f, "%s\n", "</div></div>");

    fclose(f);

    free(videoData);
}


void createHTML(){
    DIR *d;
    struct dirent *dir;
    d = opendir(PATH_XML);
    char filename[512];
    struct file_data* htmlHeaderData = file_load(HTML_HEADER_PATH);
    struct file_data* htmlFootData = file_load(HTML_FOOT_PATH);
    int contador = 0;
    int bloqueCerrado = 1;

    FILE* f = fopen("../src/serverroot/index.html", "w");
    fprintf(f, "%s\n", (char*)htmlHeaderData->data);
    fclose(f);

    if(d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                sprintf(filename, "%s/%s", PATH_XML, dir->d_name);
                if (strstr(filename, ".xml") != NULL) {

                    if(contador == 0)
                        bloqueCerrado = 0;

                    struct file_data *fileData = file_load(filename);
                    parseData(fileData->data, contador);
                    file_free(fileData);

                    if(contador == 5) {
                        bloqueCerrado = 1;
                        contador = 0;
                    }
                    else
                        contador++;
                }
            }
        }
    }

    f = fopen("../src/serverroot/index.html", "a");
    if(!bloqueCerrado)
        fprintf(f, "%s\n", "</div></div>");
    fprintf(f, "%s", (char*)htmlFootData->data);
    fclose(f);
}