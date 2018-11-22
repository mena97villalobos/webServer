//
// Created by bryan on 16/11/18.
//

#ifndef WEBSERVER_HTMLCREATOR_H
#define WEBSERVER_HTMLCREATOR_H

struct xmlVideo{
    char* nombre;
    char* descripcion;
    char* tamanno;
    char* fecha;
    char* previewPath;
};


void mainCreateHTML(char*);
struct xmlVideo* parseData(char*);
void createIndexHTML(struct xmlVideo*, int);
void createAdminHTML(struct xmlVideo*);

#endif //WEBSERVER_HTMLCREATOR_H
