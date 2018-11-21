#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/file.h>
#include <fcntl.h>
#include "net.h"
#include "file.h"
#include "mime.h"
#include "cache.h"
#include <openssl/md5.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <htmlCreator.h>
#include <pthread.h>

#define PORT "3940"  // the port users will be connecting to
#define PORT_MODIFY "5555"
#define SERVER_FILES "../src/serverfiles"
#define SERVER_ROOT "../src/serverroot"
#define ADMIN_PASS "admin"
#define ADMIN_USER "admin"
#define PATH_BITACORA "../src/serverfiles/serverLog.log"

struct datos_thread{
    int fd;
    struct cache* cachethread;
    char * puerto;
};

void annadirEntradaBitacora(char* entrada){
    FILE* fd = fopen(PATH_BITACORA, "a");
    flock(fd, LOCK_EX);

    fprintf(fd, "%s\n", entrada);

    flock(fd, LOCK_UN);
    fclose(fd);
}





void separar_parametros(char valores_param [][1000], char *string_parametros){
    char * parametros[10];

    parametros[0] = strtok(string_parametros,"&");
    int i =0;
    while(parametros[i]!= NULL){
        i++;
        parametros[i] = strtok(NULL,"&");

    }
    for(int j = 0; j<i;j++){
        strcpy(valores_param[j],strtok(parametros[j],"="));
        strcpy(valores_param[j],strtok(NULL,"="));
    }
}

int verificar_login(char * credenciales){
    char credenciales_separadas [2][1000];

    separar_parametros(credenciales_separadas,credenciales);


    return (!strcmp(credenciales_separadas[0],ADMIN_USER) && !strcmp(credenciales_separadas[1],ADMIN_PASS));

}


void modificar_info_video(char * parametros) {
    char valores_parametros_tmp[10][1000];
    char valores_parametros_tmpvid[1000];
    char valores_xml[10][1000];
    char path_archivoxml_modificar[100];
    char path_archivomp4_modificar[100];
    char path_nuevoxml[100];
    char path_nuevomp4[100];
    //char
    char tmp[10];
    char path_tmp[100];
    FILE *fp;
    FILE *fp_tmp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    separar_parametros(valores_parametros_tmp, parametros);
    strcpy(valores_parametros_tmpvid,valores_parametros_tmp[0]);

    sprintf(tmp,"%s.xml","tmp"); //TODO el archivo temporal puede joder la vara

    sprintf(path_tmp,"%s/%s",SERVER_ROOT,tmp);
    sprintf(valores_parametros_tmp[0], "%s.xml", valores_parametros_tmp[0]);
    sprintf(valores_parametros_tmpvid, "%s.mp4", valores_parametros_tmpvid);
    sprintf(valores_xml[0], "    <nombre>%s</nombre>\n",valores_parametros_tmp[1]);
    sprintf(valores_xml[1], "    <descripcion>%s</descripcion>\n",valores_parametros_tmp[2]);
    sprintf(valores_xml[2], "    <fecha>%s</fecha>\n",valores_parametros_tmp[3]);
    sprintf(path_archivoxml_modificar, "%s/%s", SERVER_ROOT, valores_parametros_tmp[0]);
    sprintf(path_archivomp4_modificar, "%s/%s", SERVER_ROOT, valores_parametros_tmpvid);
    sprintf(path_nuevoxml, "%s/%s.%s",SERVER_ROOT,valores_parametros_tmp[1],"xml");
    sprintf(path_nuevomp4, "%s/%s.%s",SERVER_ROOT,valores_parametros_tmp[1],"mp4");

    fp = fopen(path_archivoxml_modificar, "r");
    fp_tmp = fopen(path_tmp,"w");

    if (fp == NULL || fp_tmp == NULL)
        perror("Error al abrir o crear el archivo");
    else {
        int contador = 0;
        int posiciones[] = {1,2,4};
        while ((read = getline(&line, &len, fp)) != -1) {
            if(contador == posiciones[0])
                fprintf(fp_tmp,"%s",valores_xml[0]);
            else if(contador == posiciones[1])
                fprintf(fp_tmp,"%s",valores_xml[1]);
            else if (contador == posiciones[2])
                fprintf(fp_tmp,"%s",valores_xml[2]);
            else {
                fprintf(fp_tmp, "%s", line);
            }
            contador++;
        }
        fclose(fp);
        fclose(fp_tmp);
        rename(path_tmp,path_archivoxml_modificar);
        rename(path_archivoxml_modificar, path_nuevoxml);
        rename(path_archivomp4_modificar, path_nuevomp4);
    }

}

void dividir_request_path(char *request_divided[], char request_path[]){
   request_divided[0]= strtok(request_path, "?");
    int i =0;
    while(request_divided[i]!= NULL){
        i++;
        request_divided[i] = strtok(NULL,"?");
    }
}

int send_response(int fd, char *header, char *content_type, void *body, int content_length) {
    char* response = calloc(content_length + 1024, sizeof(char));
    time_t t1 = time(NULL);
    struct tm *ltime = localtime(&t1);

    int response_length = sprintf(response,
        "%s\n"
        "Date: %s" // asctime adds its own newline
        "Connection: close\n"
        "Content-Length: %d\n"
        "Content-Type: %s\n"
        "\n", // End of HTTP header
        header,
        asctime(ltime),
        content_length,
        content_type
    );

    memcpy(response + response_length, body, content_length);
    ssize_t rv = send(fd, response, response_length + content_length, 0);
    free(response);

    if (rv < 0) {
        perror("send");
    }
    return (int)rv;
}

void resp_404(int fd) {
    char filepath[4096];
    struct file_data *filedata; 
    char *mime_type;

    sprintf(filepath, "%s/404.html", SERVER_FILES);
    filedata = file_load(filepath);

    if (filedata == NULL) {
        fprintf(stderr, "cannot find system 404 file\n");
        exit(3);
    }

    mime_type = mime_type_get(filepath);

    send_response(fd, "HTTP/1.1 404 NOT FOUND", mime_type, filedata->data, filedata->size);

    file_free(filedata);
}


int get_file_or_cache(int fd, struct cache *cache, char *filepath)
{
    struct file_data *filedata; 
    struct cache_entry *cacheent;
    char *mime_type;

    // Try to find the file
    cacheent = cache_get(cache, filepath);

    if (cacheent != NULL) {
        // Found it in the cache

        send_response(fd, "HTTP/1.1 200 OK",  cacheent->content_type, cacheent->content, cacheent->content_length);

    } else {
        filedata = file_load(filepath);

        if (filedata == NULL) {
            return -1; // failure
        }

        mime_type = mime_type_get(filepath);

        send_response(fd, "HTTP/1.1 200 OK",  mime_type, filedata->data, filedata->size);

        // Save it for next time
        cache_put(cache, filepath, mime_type, filedata->data, filedata->size);

        file_free(filedata);
    }

    return 0; // success
}

void get_file(int fd, struct cache *cache, char *request_path, char * puerto, int esAdmin)
{
    char filepath[4096];
    int status;

    // Try to find the file
    sprintf(filepath, "%s%s", SERVER_ROOT, request_path);
    status = get_file_or_cache(fd, cache, filepath);

    if (status == -1) {

        if(strcmp(puerto,"3940") == 0)
            sprintf(filepath, "%s%s/index.html", SERVER_ROOT, request_path);
        else{
            if(!esAdmin)
                sprintf(filepath, "%s%s/adminLogin.html", SERVER_ROOT, request_path);
            else
                sprintf(filepath, "%s%s/admin.html", SERVER_ROOT, request_path);
        }

        status = get_file_or_cache(fd, cache, filepath);

        if (status == -1) {
            resp_404(fd);
            return;
        }
    }
}

void post_save(int fd, char *body)
{
    char *status;

    // Open the file
    int file_fd = open("data.txt", O_CREAT|O_WRONLY, 0644);

    if (file_fd >= 0) {
        // Exclusive lock to keep processes from trying to write the file at the
        // same time. This is only necessary if we've implemented a
        // multiprocessed version with fork().
        flock(file_fd, LOCK_EX);

        // Write body
        write(file_fd, body, strlen(body));

        // Unlock
        flock(file_fd, LOCK_UN);

        // Close
        close(file_fd);

        status = "ok";
    } else {
        status = "fail";
    }

    // Now send an HTTP response

    char response_body[128];

    sprintf(response_body, "{\"status\": \"%s\"}\n", status);

    send_response(fd, "HTTP/1.1 200 OK", "application/json", response_body, strlen(response_body));
}

char *find_start_of_body(char *header)
{
    char *p;

    p = strstr(header, "\n\n");

    if (p != NULL) return p;

    p = strstr(header, "\r\n\r\n");

    if (p != NULL) return p;

    p = strstr(header, "\r\r");

    return p;
}

/**
 * Handle HTTP request and send response
 */
int handle_http_request(int fd, struct cache *cache, char * puerto)
{
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];
    char *p;
    char request_type[8]; // GET or POST
    char request_path[1024]; // /info etc.
    char request_protocol[128]; // HTTP/1.1
    char request_path_copy[1024];
    char * request_path_div [80];

    // Read request
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0) {
        perror("recv");
        return 1;
    }
    if(bytes_recvd > 0) {
        request[bytes_recvd] = '\0';
        p = find_start_of_body(request);
        if (p == NULL) {
            printf("Could not find end of header\n");
            exit(1);
        }
        char *body = p;
        sscanf(request, "%s %s %s", request_type, request_path,
               request_protocol);

        char entradaLog[1200];
        sprintf(entradaLog, "%s %s %s %s", "REQUEST: ", request_type, request_path, request_protocol);
        annadirEntradaBitacora(entradaLog);
        printf("REQUEST: %s %s %s\n", request_type, request_path, request_protocol);

        strcpy(request_path_copy, request_path);
        dividir_request_path(request_path_div, request_path_copy);
        if (strcmp(request_type, "GET") == 0) {
            if (strcmp(request_path_div[0], "/admin.html") == 0) { //TODO cambiar al archivo admin.html
                annadirEntradaBitacora("Error intento de entrar como administrador");
                resp_404(fd);
            }
            else if(strcmp(request_path_div[0],"/ingresar") == 0){
               int tienePermiso = verificar_login(request_path_div[1]);
               get_file(fd, cache, "/", puerto, tienePermiso);
            }
            else
                get_file(fd, cache, request_path, puerto,(int)NULL);
        }
        else if (strcmp(request_type, "POST") == 0) {
            // Endpoint "/save"
            if (strcmp(request_path, "/save") == 0) {
                post_save(fd, body);
            }
            else if (strcmp(request_path_div[0], "/modificarXML") == 0) {
                modificar_info_video(request_path_div[1]); //Envia los parametros de modificacion
                get_file(fd, cache, "/", puerto, 1); //No es necesario este valor
            }
            else {
                resp_404(fd);
            }
        }
        else {
            fprintf(stderr, "unknown request type \"%s\"\n", request_type);
            annadirEntradaBitacora("Error tipo de petición desconocida");
        }
        return 0;
    }
    else{
        return 1;
    }
}

void* nueva_peticion(void* datos){ //TODO retorna void o puntero a void?
    int fd = ((struct datos_thread*)datos) ->fd;
    struct cache* cache = ((struct datos_thread*)datos) ->cachethread;
    char * puerto = ((struct datos_thread*)datos) ->puerto;
    while(1) {
        int retorno = handle_http_request(fd, cache, puerto);
        if(retorno)
            break;
    }
    close(fd);
}


int main(void){
    int newfd;  // listen on sock_fd, new connection on newfd
    int newfdmodify;
    struct sockaddr_storage their_addr; // connector's address information
    struct sockaddr_storage addr_modify;
    char s[INET6_ADDRSTRLEN];
    char smodify[INET6_ADDRSTRLEN];
    char entradaLog[INET6_ADDRSTRLEN + 30];
    pthread_t tid;
    pid_t pid;
    //Creación del archivo index.html con los archivos disponibles cuando arranca el servidor
    mainCreateHTML();
    annadirEntradaBitacora("Archivos index.html y admin.html actualizados\n");
    //struct cache *cache = cache_create(10, 0);
    struct cache *cachemodify = cache_create(10,0);
    pid = fork();
    if(pid>0) {
        int listenfd = get_listener_socket(PORT);

        if (listenfd < 0) {
            fprintf(stderr, "webserver: fatal error getting listening socket\n");
            annadirEntradaBitacora("webserver: fatal error getting listening socket");
            exit(1);
        }

        printf("webserver: waiting for connections on port %s...\n", PORT);

        while (1) {
            socklen_t sin_size = sizeof their_addr;
            struct cache *cache = cache_create(10, 0);
            newfd = accept(listenfd, (struct sockaddr *) &their_addr, &sin_size);
            if (newfd == -1) {
                annadirEntradaBitacora("Error con el proceso de aceptación");
                perror("accept");
                continue;
            }
            inet_ntop(their_addr.ss_family,
                      get_in_addr((struct sockaddr *) &their_addr),
                      s, sizeof s);
            printf("server: got connection from %s\n", s);
            sprintf(entradaLog, "%s%s\n", "server: got connection from %s\n", s);
            annadirEntradaBitacora(entradaLog);


            struct datos_thread * datos_th = malloc(sizeof(struct datos_thread));
            datos_th->fd = newfd;
            datos_th->cachethread = cache;
            datos_th-> puerto= PORT;
            pthread_create(&tid,NULL,nueva_peticion,(void *) datos_th);
        }
    }
    else if(pid < 0)
        annadirEntradaBitacora("Error Creando FORK");
    else{
        int listenfdModify = get_listener_socket(PORT_MODIFY);

        if (listenfdModify < 0) {
            fprintf(stderr, "webserver: fatal error getting listening socket\n");
            annadirEntradaBitacora("webserver: fatal error getting listening socket");
            exit(1);
        }

        printf("webserver: waiting for connections on port %s...\n", PORT_MODIFY);

        while (1) {
            socklen_t sin_size = sizeof addr_modify;
            newfdmodify = accept(listenfdModify, (struct sockaddr *) &addr_modify, &sin_size);
            if (newfdmodify == -1) {
                annadirEntradaBitacora("Error con el proceso de aceptación");
                continue;
            }
            inet_ntop(addr_modify.ss_family,
                      get_in_addr((struct sockaddr *) &addr_modify),
                      smodify, sizeof smodify);


            sprintf(entradaLog, "%s%s\n", "server: got connection from %s\n", smodify);
            annadirEntradaBitacora(entradaLog);

            printf("server: got connection from %s\n", smodify); //TODO bitacora
            handle_http_request(newfdmodify, cachemodify,PORT_MODIFY);
            close(newfdmodify);
        }
    }
    return 0;
}

