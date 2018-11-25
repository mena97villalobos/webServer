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
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>


#define PORT "3940"  // the port users will be connecting to
#define PORT_MODIFY "5555"
#define SERVER_FILES "../src/serverfiles"
#define SERVER_ROOT "../src/serverroot"
#define ADMIN_PASS "admin"
#define ADMIN_USER "admin"
#define PATH_BITACORA "../src/serverfiles/serverLog.log"
#define PATH_MD5_SUM "../src/serverfiles/sumaMD5.txt"

struct datos_thread{
    int fd;
    struct cache* cachethread;
    char * puerto;
    sem_t* sem;
    char* timeUpdate;
};

//GLOBAL
char tiempoString[26];


//Variables Compartidas
long  bytesTransferidos = 0;
long  solicitudesAtendidas = 0;
double tiempo = 0;
sem_t lockBytes;
sem_t lockSolicitudes;
sem_t lockTiempo;


//Variables Compartidas

struct wrapper{
    sem_t lockClientes;

    sem_t lockThreads;

    long  clientesDistintos;
    long threadsCreados;
    int contadorClientes;
    char listaClientes[10000][100];

};


char *segmentoMemoria;
struct wrapper * variablesComp;

void establecerMemoriaCompartida(){

    key_t key;
    int shmid;

    key = ftok("shmfile",65);
    shmid = shmget(key,sizeof(variablesComp),0666|IPC_CREAT); //0666

    segmentoMemoria = shmat(shmid, (void *)0, 0);
    variablesComp = (struct wrapper*)segmentoMemoria;
    variablesComp->clientesDistintos = 0;
    variablesComp->threadsCreados = 0;
    variablesComp->contadorClientes = 0;

}

void crearCandado(){
    sem_init(&variablesComp->lockClientes, 1,1);
    sem_init(&variablesComp->lockThreads,1,1);
    sem_init(&lockBytes,0,1);
    sem_init(&lockSolicitudes,0,1);
    sem_init(&lockTiempo,0,1);
}

void modificarTiempo(double time){
    sem_wait(&lockTiempo);

        tiempo = tiempo + time;

    sem_post(&lockTiempo);
}

void modificarThreads(){
    sem_wait(&variablesComp->lockThreads);
        variablesComp->threadsCreados+=1;
    sem_post(&variablesComp->lockThreads);
}

int clienteExiste(char cliente[]){
    if(variablesComp->contadorClientes == 0) {
        strcpy(variablesComp->listaClientes[variablesComp->contadorClientes], cliente);
        variablesComp->contadorClientes= variablesComp->contadorClientes+1;

        return 0;
    }else{
        for(int i =0; i<variablesComp->contadorClientes;i++){
            if(!strcmp(variablesComp->listaClientes[i],cliente))
                return 1;
        }
        strcpy(variablesComp->listaClientes[variablesComp->contadorClientes], cliente);
        variablesComp->contadorClientes= variablesComp->contadorClientes+1;

    }
    return 0;
}

void modificarClientes(char cliente[]){
    sem_wait(&variablesComp->lockClientes);
    if(!clienteExiste(cliente))
        variablesComp->clientesDistintos+=1;
    sem_post(&variablesComp->lockClientes);
}

void modificarBytes(int nuevosBytes){
    sem_wait(&lockBytes);
    bytesTransferidos= bytesTransferidos + nuevosBytes;
    sem_post(&lockBytes);
}

void modificarSolicitudes(){
    sem_wait(&lockSolicitudes);
    solicitudesAtendidas+=1;
    sem_post(&lockSolicitudes);
}

void annadirEntradaBitacora(char* entrada){
    time_t t1 = time(NULL);
    struct tm *ltime = localtime(&t1);
    FILE* fd = fopen(PATH_BITACORA, "a");
    flock(fd, LOCK_EX);
    fprintf(fd, "%s%s\n", asctime(ltime), entrada);
    flock(fd, LOCK_UN);
    fclose(fd);
}

void calculateMD5(sem_t* sem, char* timeUpdate){
    while(1) {
        char ultimaSuma[37];
        char sumaActual[37];
        char c;
        int len = 0;
        FILE* fd = fopen(PATH_MD5_SUM, "r");
        flock(fd, LOCK_EX);
        fseek(fd, -36, SEEK_END);
        c = (char)fgetc(fd);
        while(len < 36) {
            ultimaSuma[len] = c;
            fseek(fd, 0, SEEK_CUR);
            len++;
            c = (char)fgetc(fd);
        }
        ultimaSuma[len] = "\0";
        system("find ../src/serverroot -type f -exec md5sum {} \\; | sort -k 2 | md5sum > "
                         "../src/serverfiles/sumaMD5.txt");
        len = 0;
        fseek(fd, -36, SEEK_END);
        c = (char)fgetc(fd);
        while(len < 36)
        {
            sumaActual[len] = c;
            fseek(fd, 0, SEEK_CUR);
            len++;
            c = (char)fgetc(fd);
        }
        sumaActual[len] = "\0";
        if(strcmp(ultimaSuma, sumaActual) == 0){
            annadirEntradaBitacora("Las sumas md5 son iguales, no hay cambios en los archivos del servidor");
        }
        else{
            time_t t1 = time(NULL);
            struct tm *ltime = localtime(&t1);
            char tiempoString[26];
            asctime_r(ltime, tiempoString);
            tiempoString[24] = tiempoString[25];
            annadirEntradaBitacora("Las sumas md5 son diferentes\n");

            //Actualizar variable compartida de tiempo
            sem_wait(sem);
            memcpy(timeUpdate, tiempoString, 24);
            sem_post(sem);

            mainCreateHTML(tiempoString);
            system("find ../src/serverroot -type f -exec md5sum {} \\; | sort -k 2 | md5sum > "
                             "../src/serverfiles/sumaMD5.txt");
        }
        flock(fd, LOCK_UN);
        fclose(fd);
        sleep(10);
    }
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
    char tmp[10];
    char path_tmp[100];
    FILE *fp;
    FILE *fp_tmp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    separar_parametros(valores_parametros_tmp, parametros);
    strcpy(valores_parametros_tmpvid,valores_parametros_tmp[0]);
    sprintf(tmp,"%s.xml","tmp");
    sprintf(path_tmp,"%s/%s",SERVER_ROOT,tmp);
    sprintf(valores_parametros_tmp[0], "%s.xml", valores_parametros_tmp[0]);
    sprintf(valores_parametros_tmpvid, "%s.mp4", valores_parametros_tmpvid);
    sprintf(valores_xml[0], "    <nombre>%s</nombre>\n",valores_parametros_tmp[1]);
    sprintf(valores_xml[1], "    <descripcion>%s</descripcion>\n",valores_parametros_tmp[3]);
    sprintf(valores_xml[2], "    <fecha>%s</fecha>\n",valores_parametros_tmp[2]);
    sprintf(path_archivoxml_modificar, "%s/%s", SERVER_ROOT, valores_parametros_tmp[0]);
    sprintf(path_archivomp4_modificar, "%s/%s", SERVER_ROOT, valores_parametros_tmpvid);
    sprintf(path_nuevoxml, "%s/%s.%s",SERVER_ROOT,valores_parametros_tmp[1],"xml");
    sprintf(path_nuevomp4, "%s/%s.%s",SERVER_ROOT,valores_parametros_tmp[1],"mp4");
    fp = fopen(path_archivoxml_modificar, "r");
    fp_tmp = fopen(path_tmp,"w");
    if (fp == NULL || fp_tmp == NULL){
        perror("Error al abrir o crear el archivo");
        remove(path_tmp);
    }
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

int send_response(int fd, char *header, char *content_type, void *body, int content_length){

    char* response = calloc(content_length + 1024, sizeof(char));
    time_t t1 = time(NULL);
    struct tm *ltime = localtime(&t1);
    int response_length = sprintf(response, "%s\nDate: %sConnection: close\nContent-Length: %d\nContent-Type: %s\n"
                                            "\n", header, asctime(ltime), content_length, content_type);
    memcpy(response + response_length, body, content_length);
    ssize_t rv = send(fd, response, response_length + content_length, 0);
    modificarBytes(response_length + content_length);

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

int get_file_or_cache(int fd, struct cache *cache, char *filepath) {
    struct file_data *filedata;
    struct cache_entry *cacheent;
    char *mime_type;

    // Try to find the file
    cacheent = cache_get(cache, filepath);
    if (cacheent != NULL) {
        // Found it in the cache
        send_response(fd, "HTTP/1.1 200 OK",  cacheent->content_type, cacheent->content, cacheent->content_length);
    }
    else {
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

void get_file(int fd, struct cache *cache, char *request_path, char * puerto, int esAdmin) {
    char filepath[4096];
    int status;

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

void post_save(int fd, char *body) {
    char *status;
    int file_fd = open("data.txt", O_CREAT|O_WRONLY, 0644);

    if (file_fd >= 0) {
        flock(file_fd, LOCK_EX);
        write(file_fd, body, strlen(body));
        flock(file_fd, LOCK_UN);
        close(file_fd);
        status = "ok";
    }
    else {
        status = "fail";
    }
    char response_body[128];
    sprintf(response_body, "{\"status\": \"%s\"}\n", status);
    send_response(fd, "HTTP/1.1 200 OK", "application/json", response_body, strlen(response_body));
}

char *find_start_of_body(char *header) {
    char *p;
    p = strstr(header, "\n\n");
    if (p != NULL) return p;
    p = strstr(header, "\r\n\r\n");
    if (p != NULL) return p;
    p = strstr(header, "\r\r");
    return p;
}

int handle_http_request(int fd, struct cache *cache, char * puerto, sem_t* sem, char* timeUpdate) {
    const int request_buffer_size = 65536; // 64K
    char request[request_buffer_size];
    char *p;
    char request_type[8];
    char request_path[1024];
    char request_protocol[128];
    char request_path_copy[1024];
    char * request_path_div [80];
    int bytes_recvd = recv(fd, request, request_buffer_size - 1, 0);

    if (bytes_recvd < 0) {
        perror("recv");
        return 1;
    }
    if(bytes_recvd > 0) {
        if(!strcmp(puerto,PORT)) {
            modificarBytes(bytes_recvd);
            modificarSolicitudes();
        }

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
        strcpy(request_path_copy, request_path);
        dividir_request_path(request_path_div, request_path_copy);

        if (strcmp(request_type, "GET") == 0) {
            if(strcmp(request_path_div[0], "/actualizarIndex") == 0){
                send_response(fd, "HTTP/1.1 200 OK", "text/plain", timeUpdate, 24);
            }
            else if (strcmp(request_path_div[0], "/admin.html") == 0) {
                annadirEntradaBitacora("Error intento de entrar como administrador");
                resp_404(fd);
            }
            else if(strcmp(request_path_div[0],"/ingresar") == 0){
                int tienePermiso = verificar_login(request_path_div[1]);
                get_file(fd, cache, "/", puerto, tienePermiso);
            }
            else if (strcmp(request_path_div[0], "/modificarXML") == 0) {
                modificar_info_video(request_path_div[1]);
                get_file(fd, cache, "/", puerto, 1);
            }
            else
                get_file(fd, cache, request_path, puerto,(int)NULL);
        }
        else if (strcmp(request_type, "POST") == 0) {
            if (strcmp(request_path, "/save") == 0) {
                post_save(fd, body);
            }
            else {
                resp_404(fd);
            }
        }
        else {
            fprintf(stderr, "unknown request type \"%s\"\n", request_type);
            annadirEntradaBitacora("Error tipo de petición desconocida");
        }
        return 1;
    }
    else{
        return 1;
    }
}

void* nueva_peticion(void* datos){
    double elapsed = 0;
    struct datos_thread* d = (struct datos_thread*)datos;
    int fd = d->fd;
    struct cache* cache = d->cachethread;
    char * puerto = d->puerto;
    time_t start, end;
    clock_t inicio = clock();
    while(1) {
        int retorno = handle_http_request(fd, cache, puerto, d->sem, d->timeUpdate);
        if(retorno)
            break;
    }
    elapsed = (double)((clock() - inicio)*1000 / CLOCKS_PER_SEC);

    modificarTiempo(elapsed/1000);
    printf("Tiempo de Ejecucion: %lf s \n",elapsed/1000);
    close(fd);
}

void imprimirMenu(){

    printf("\n 1) Ver Hora de Inicio\n 2) Ver Cantidad de Bytes Transferidos\n 3) Velocidad Promedio en Atender Solicitudes\n 4) Ver Cantidad de Clientes y Administradores que Han Consultado\n"
           " 5) Ver Cantidad de Solicitudes Atendidas de Clientes\n 6) Ver Cantidad de Threads Creados \n");

}

void realizarAccionAdministrador(int opcion){
    switch(opcion){
        case 1:
            printf("\nServidor Iniciado a las: %s\n",tiempoString);
            break;
        case 2:
            printf("\nBytes Transferidos en Total: %ld \n",bytesTransferidos);
            break;
        case 3:
            printf("\nLas solicitudes son atendidas en un tiempo promedio de: %lf \n",tiempo/solicitudesAtendidas);
            break;
        case 4:
            printf("\nTotal de Clientes y Administradores Conectados: %ld\n",variablesComp->clientesDistintos);
            break;
        case 5:
            printf("\nTotal de Solicitudes Atendidas: %ld\n",solicitudesAtendidas);
            break;
        case 6:
            printf("\nTotal de Threads Creados: %ld\n",variablesComp->threadsCreados);
            break;
        default:
            printf("\nOpción. Intente de nuevo.\n");
    }
}

void* menu_administrador(){
    int opcionSeleccionada;
    while(1){
        imprimirMenu();
        printf("\nSeleccione una Opción: ");
        scanf("%i",&opcionSeleccionada);

        realizarAccionAdministrador(opcionSeleccionada);

        getchar();
    }
}

int main(void){
    int newfd;
    int newfdmodify;
    struct sockaddr_storage their_addr;
    struct sockaddr_storage addr_modify;
    char s[INET6_ADDRSTRLEN];
    char smodify[INET6_ADDRSTRLEN];
    char entradaLog[INET6_ADDRSTRLEN + 30];
    pthread_t tid;
    pthread_t tidadmin;
    pthread_t menuAdmin;
    pid_t pid;

    time_t t1 = time(NULL);
    struct tm *ltime = localtime(&t1);

    asctime_r(ltime, tiempoString);
    tiempoString[24] = tiempoString[25];

    //Creación del archivo index.html con los archivos disponibles cuando arranca el servidor
    mainCreateHTML(tiempoString);
    annadirEntradaBitacora("Archivos index.html y admin.html actualizados\n");

    //Semaforo para ultima vez actualizado
    char* timeUpdate;
    key_t shmkey;                 //shared memory key
    int shmid;                    //shared memory id
    sem_t *sem;                   //synch semaphore
    shmkey = ftok("/dev/null", 5);
    shmid = shmget(shmkey, 26*sizeof(char), 0644 | IPC_CREAT);
    if (shmid < 0){
        perror("shmget\n");
        exit (1);
    }
    timeUpdate = (char *) shmat(shmid, NULL, 0);
    memcpy(timeUpdate, tiempoString, 24);
    sem = sem_open ("pSem", O_CREAT, 0644, 1);
    //


    // struct cache *cache = cache_create(10, 0);
    struct cache *cachemodify = cache_create(10,0);

    establecerMemoriaCompartida();
    crearCandado();

    pid = fork();
    if(pid == 0){
        system("find ../src/serverroot -type f -exec md5sum {} \\; | sort -k 2 | md5sum > "
                         "../src/serverfiles/sumaMD5.txt");
        calculateMD5(sem, timeUpdate);
    }
    else if(pid > 0){
        pid = fork();
        if (pid > 0) {

            pthread_create(&menuAdmin,NULL,menu_administrador,NULL);

            int listenfd = get_listener_socket(PORT);

            if (listenfd < 0) {
                fprintf(stderr, "webserver: fatal error getting listening socket\n");
                annadirEntradaBitacora("webserver: fatal error getting listening socket");
                exit(1);
            }
            //printf("webserver: waiting for connections on port %s...\n", PORT);
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
                sprintf(entradaLog, "%s%s\n", "server: got connection from \n", s);
                annadirEntradaBitacora(entradaLog);

                modificarClientes(s);

                struct datos_thread *datos_th = malloc(sizeof(struct datos_thread));
                datos_th->fd = newfd;
                datos_th->cachethread = cache;
                datos_th->puerto = PORT;
                datos_th->sem = sem;
                datos_th->timeUpdate = timeUpdate;
                modificarThreads();
                pthread_create(&tid, NULL, nueva_peticion, (void *) datos_th);
            }
        } else if (pid < 0)
            annadirEntradaBitacora("Error Creando FORK");
        else {
            int listenfdModify = get_listener_socket(PORT_MODIFY);
            if (listenfdModify < 0) {
                fprintf(stderr, "webserver: fatal error getting listening socket\n");
                annadirEntradaBitacora("webserver: fatal error getting listening socket");
                exit(1);
            }
            //printf("webserver: waiting for connections on port %s...\n", PORT_MODIFY);
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
                sprintf(entradaLog, "%s%s\n", "server: got connection from \n", smodify);
                annadirEntradaBitacora(entradaLog);

                modificarClientes(smodify);
                struct datos_thread *datos_th = malloc(sizeof(struct datos_thread));
                datos_th->fd = newfdmodify;
                datos_th->cachethread = cachemodify;
                datos_th->puerto = PORT_MODIFY;
                datos_th->sem = sem;
                datos_th->timeUpdate = timeUpdate;

                modificarThreads();
                pthread_create(&tidadmin, NULL, nueva_peticion, (void *) datos_th);
            }
        }
    }
    return 0;
}
