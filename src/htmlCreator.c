#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory.h>
#include <regex.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
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
#define HTML_TMPSS "../src/serverroot/tmpss.html"


void createAdminHTML(struct xmlVideo* videoData){
    FILE* f = fopen("../src/serverroot/admin.html", "a");

    fprintf(f, "%s\n", "<br></br><div class=\"row\"><div class=\"col-md-6\">\n");
    fprintf(f, "%s%s%s\n", "<img width =\"250\" height= \"250\" src=\"", videoData->previewPath, "\">\n");
    fprintf(f, "%s\n", "</div><div class=\"col-md-6\"><h2 class=\"pb-3 align-left mbr-fonts-style display-2\">"
                       "</h2><div><div class=\"icon-block pb-3\">"
                       "<h4 class=\"icon-block__title align-left mbr-fonts-style display-5\">");
    fprintf(f, "%s%s%s%s%s%s\n", videoData->nombre, "</h4></div><div class=\"icon-contacts pb-3\">"
                                            "<h5 class=\"align-left mbr-fonts-style display-7\"></h5>\n"
                                            "<p class=\"mbr-text align-left mbr-fonts-style display-7\"></p></div>\n"
                                            "</div><div><div data-form-alert=\"\" "
                                            "hidden=\"\">Solicitud de modificación enviada!</div><form method=\"get\" "
                                            "action=\"/modificarXML\" enctype=\"multipart/form-data\"><div class=\"row\">\n"
                                            "<div class=\"col-md-6 multi-horizontal\">\n"
                                            "<input name =\"nombOriginal\" type = \"hidden\"  value=\"",videoData->nombre,
                                            "\" ><input type=\"text\" class=\"form-control input\" data-form-field=\"Titulo\" "
                                            "placeholder=\"", videoData->nombre, "\" required=\"\" name = \"nombvideo\">");
    fprintf(f, "%s%s%s%s%s\n", "</div><div class=\"col-md-6 multi-horizontal\">\n"
                       "<input type=\"date\" name = \"fechavideo\" class=\"form-control input\" value=\"", videoData->fecha,
                       "\">\n</div>\n<div class=\"col-md-12\">\n<textarea class=\"form-control input\" name=\"descvideo\" rows=\"3\" "
                       "data-form-field=\"Descripción\"  style=\"resize:none\"  placeholder=\"", videoData->descripcion,
                       "\"></textarea></div>\n<div class=\"input-group-btn col-md-12\" style=\"margin-top: 10px;\">\n"
                       "<button href=\"\" type=\"submit\" class=\"btn btn-primary btn-form display-4\">MODIFICAR</button>\n"
                       "</div></div></form></div></div></div> <br></br>");
    fclose(f);

}

void createIndexHTML(struct xmlVideo* videoData, int indexBloque){
    FILE* f = fopen("../src/serverroot/index.html", "a");

    if(indexBloque == 0)
        fprintf(f, "%s\n", "<div class=\"container-fluid\"> <div class=\"media-container-row\">");

    fprintf(f, "%s%s%s", "<div class=\"card p-3 col-12 col-md-6 col-lg-2\">"
                         "<div class=\"card-wrapper\"><div class=\"card-img\"><a href=\"", videoData->nombre,
            ".mp4\"><img src=\"");
    fprintf(f, "%s", videoData->previewPath);
    fprintf(f, "%s", "\"></a></div><div class=\"card-box\"><h4 class=\"card-title pb-3 mbr-fonts-style display-7\">\n");
    fprintf(f, "%s", videoData->nombre);
    fprintf(f, "%s", "</h4><p class=\"mbr-text mbr-fonts-style display-7\">\n");
    fprintf(f, "Descripción: %s<br>\nTamaño: %s<br>\nFecha: %s<br>\n", videoData->descripcion, videoData->tamanno, videoData->fecha);
    fprintf(f, "%s", "</p>\n</div>\n</div></div>");

    if(indexBloque == 5)
        fprintf(f, "%s\n", "</div></div>");

    fclose(f);
}

void createSlideshowHTML(struct xmlVideo* videoData, int contador){
    FILE* fSlide= fopen("../src/serverroot/slideshow.html","a");
    FILE* ftmp= fopen(HTML_TMPSS,"a");

    switch(contador){
        case 0:
            fprintf(fSlide,"%s\n","<div id=\"myCarousel\" class=\"carousel slide\" data-ride=\"carousel\">");
            fprintf(fSlide,"%s\n","<ol class=\"carousel-indicators\">");
            fprintf(fSlide,"<li data-target=\"#myCarousel\" data-slide-to=\"%i\" class=\"active\"></li>\n",contador);

            fprintf(ftmp,"%s\n","<div class=\"carousel-inner\">");
            fprintf(ftmp,"<div class=\"carousel-item active\">\n"
                         " <a href= \"%s.mp4\"> <img width =\"550\" height= \"410\"  class=\"d-block w-100\" src=\"%s\"></a>\n""<div class=\"carousel-caption\">\n"
                         "   <p>Titulo: %s<br>Descripcion: %s<br>Tamaño: %s<br>Fecha: %s\n</p>\n"
                         "      </div>\n</div>\n",videoData->nombre,videoData->previewPath,videoData->nombre,videoData->descripcion,
                    videoData->tamanno,videoData->fecha);
                         //"</div>\n",videoData->nombre,videoData->previewPath);
            break;

        default:
            fprintf(fSlide,"<li data-target=\"#myCarousel\" data-slide-to=\"%i\"></li>\n",contador);

            fprintf(ftmp,"<div class=\"carousel-item\">\n"
                         " <a href= \"%s.mp4\"> <img width =\"550\" height= \"410\" class=\"d-block w-100\" src=\"%s\"></a>\n"
                         "<div class=\"carousel-caption\">\n"
                         "   <p>Titulo: %s<br>Descripcion: %s<br>Tamaño: %s<br>Fecha: %s\n</p>\n"
                         "      </div>\n</div>\n",videoData->nombre,videoData->previewPath,videoData->nombre,videoData->descripcion,
                         videoData->tamanno,videoData->fecha);
    }
    fclose(fSlide);
    fclose(ftmp);
}

void createFootSlideshow(FILE * fslide){

    fprintf(fslide,"%s\n","<a class=\"carousel-control-prev\" href=\"#myCarousel\" role=\"button\" data-slide=\"prev\">\n"
                          "    <span class=\"carousel-control-prev-icon\" aria-hidden=\"true\"></span>\n"
                          "    <span class=\"sr-only\">Previous</span>\n"
                          "  </a>\n"
                          "  <a class=\"carousel-control-next\" href=\"#myCarousel\" role=\"button\" data-slide=\"next\">\n"
                          "    <span class=\"carousel-control-next-icon\" aria-hidden=\"true\"></span>\n"
                          "    <span class=\"sr-only\">Next</span>\n"
                          "  </a>");
    fprintf(fslide,"%s\n","</div>");

}

struct xmlVideo* parseData(char* datos){
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
                    videoData->nombre = calloc((size_t)(offset + 1), sizeof(char));
                    memcpy(videoData->nombre, &datos[pmatch[1].rm_so], offset);
                    break;
                case 1:
                    videoData->descripcion = calloc((size_t)(offset + 1), sizeof(char));
                    memcpy(videoData->descripcion, &datos[pmatch[1].rm_so], offset);
                    break;
                case 2:
                    videoData->tamanno = calloc((size_t) (offset + 1), sizeof(char));
                    memcpy(videoData->tamanno, &datos[pmatch[1].rm_so], offset);
                    break;
                case 3:
                    videoData->fecha = calloc((size_t)(offset + 1), sizeof(char));
                    memcpy(videoData->fecha, &datos[pmatch[1].rm_so], offset);
                    break;
                case 4:
                    videoData->previewPath = calloc((size_t)(offset + 1), sizeof(char));
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
    return videoData;


}

void mainCreateHTML(char* time){
    DIR *d;
    struct dirent *dir;
    d = opendir(PATH_XML);
    char filename[512];
    struct file_data* htmlHeaderData = file_load(HTML_HEADER_PATH);
    struct file_data* htmlFootData = file_load(HTML_FOOT_PATH);
    struct file_data* htmltmp;
    int contador = 0;
    int bloqueCerrado = 1;
    struct xmlVideo* datosVideo;

    FILE* fIndex = fopen("../src/serverroot/index.html", "w");
    fprintf(fIndex, "%s\n", (char*)htmlHeaderData->data);
    fclose(fIndex);

    FILE* fslide = fopen("../src/serverroot/slideshow.html","w");
    fprintf(fslide, "%s\n", (char*)htmlHeaderData->data);
    fclose(fslide);

    FILE* ftmp = fopen(HTML_TMPSS,"w");
    fclose(ftmp);

    FILE* fAdmin = fopen("../src/serverroot/admin.html", "w");
    fprintf(fAdmin, "%s%s\n", (char*)htmlHeaderData->data,
            "<section class=\"mbr-section form4 cid-r9XKgEVrj2\" id=\"form4-9\"><div class=\"container\">");
    fclose(fAdmin);

    if(d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_type == DT_REG) {
                sprintf(filename, "%s/%s", PATH_XML, dir->d_name);
                if (strstr(filename, ".xml") != NULL) {

                    if(contador == 0)
                        bloqueCerrado = 0;

                    struct file_data *fileData = file_load(filename);
                    datosVideo = parseData(fileData->data);
                    createIndexHTML(datosVideo, contador);
                    createAdminHTML(datosVideo);
                    createSlideshowHTML(datosVideo,contador);
                    free(datosVideo);
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

    fIndex = fopen("../src/serverroot/index.html", "a");
    fAdmin = fopen("../src/serverroot/admin.html", "a");
    fslide = fopen("../src/serverroot/slideshow.html","a");
    ftmp = fopen(HTML_TMPSS,"a");

    if(!bloqueCerrado)
        fprintf(fIndex, "%s\n", "</div></div>");

    fprintf(fIndex, "\n%s%s%s\n", "<script>"
                                  "\nsetInterval(actualizar, 10000);\nvar lastUpdate = \"", time,
                                  "\";\nfunction actualizar(){"
                                  "\n\tvar request = new XMLHttpRequest();\n"
                                  "\trequest.open('GET', \"/actualizarIndex\");\n"
                                  "\trequest.responseType = 'text';\n\trequest.send();\n"
                                  "\trequest.onload = function(){\n"
                                  "\t\tvar lastUpdateServer = request.response;\n"
                                  "\t\tif(lastUpdate !== lastUpdateServer){\n"
                                  "\t\t\tif(window.confirm(\"Se detectaron nuevos cambios en el servidor, recargar?\"))"
                                  "{\n\t\t\t\tlocation.reload();\n\t\t\t}\n\t\t\telse{\n\t\t\t\tlastUpdate = lastUpdateServer;\n}\n"
                                  "\t\t}\n"
                                  "\t};\n}</script>");

    fprintf(fIndex,"%s", "<a class=\" botonGuardar \" href=\"slideshow.html\" >Ir a Slideshow</a> <br> <br>");
    fprintf(fIndex, "%s", (char*)htmlFootData->data);

    fprintf(fAdmin, "%s%s", "</div>\n</section>", (char*)htmlFootData->data);

    fprintf(ftmp,"%s\n","</div>");
    fprintf(fslide,"%s\n","</ol>");

    fclose(ftmp);

    htmltmp = file_load(HTML_TMPSS);
    fprintf(fslide,"%s\n",(char*)htmltmp->data);

    createFootSlideshow(fslide);


    fprintf(fslide, "%s\n",(char*)htmlFootData->data);


    fclose(fIndex);
    fclose(fAdmin);
    fclose(fslide);

    remove(HTML_TMPSS);

}