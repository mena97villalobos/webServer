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


void createHTML();

#endif //WEBSERVER_HTMLCREATOR_H
