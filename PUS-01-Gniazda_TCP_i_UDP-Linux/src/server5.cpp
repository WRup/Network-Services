#include<iostream>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <sys/socket.h>  

#include <netinet/in.h>  // htons, htonl, ntohs, ntohl
#include <arpa/inet.h>   // inet_pton, inet_ntop

#include <dirent.h>

using namespace std;

void *thread_procedure(void*);
void *create_html(char* html);
char* getFilename(char* fileName, char* fullPath);

const int USR_MAX = 8; // Maksymalna liczba uzytkownikow naraz
const int SIZE = 1024;
const int HTML_SIZE = 5 * 1024; // 5KB

char html[HTML_SIZE];

int main(int argc, char** argv){
	
    int fdlisten, fdsock;
    sockaddr_in server_address, client_address;
    fd_set aset, rset;
    int ret;
    socklen_t cli_len;
    
    if(argc < 2){
        cerr << "Bad invocation, should be: " << argv[0] << " <port>" << endl;
        exit(EXIT_FAILURE);
    }
    
    // Utworzenie gniazda TCP nasłuchującego:
    fdlisten = socket(AF_INET, SOCK_STREAM, 0);
    if(fdlisten < 0){
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    
    // Ustawianie struktury adresowej serwera:
    memset((void*)&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));
    
    // Łączenie nazwy z gniazdem:
    ret = bind(fdlisten, (sockaddr*)&server_address, sizeof(server_address));
    if(ret < 0){
        perror("bind()");
        exit(EXIT_FAILURE);
    }
    
    // Nasłuchiwanie nadchodzących połączeń:
    ret = listen(fdlisten, USR_MAX);
    if(ret < 0){
        perror("listen()");
        exit(EXIT_FAILURE);
    }
    
    cout << "Serwer nasluchuje nadchodzace polaczenia..." << endl;
    
    // Zerowanie deskryptorów:
    FD_ZERO(&aset);
    FD_SET(fdlisten, &aset);
    
    // Generowanie kodu HTML strony:
    create_html(html);
    
    // Główna pętla serwera:
    while(1){
        
        rset = aset;
        
        // Oczekiwanie na gniazda gotowe do czytania:
        ret = select(FD_SETSIZE, &rset, NULL, NULL, NULL);
        if(ret < 0){
            perror("select()");
            close(fdlisten);
            exit(EXIT_FAILURE);
        }
            
            // Jeśli gotowe do czytania (bufor posiada dane do czytania):
            if(FD_ISSET(fdlisten, &rset)){
                
                // Następuje akceptacja nadchodzacych połączeń:
                cli_len = sizeof(client_address);
                fdsock = accept(fdlisten, (sockaddr*)&client_address, &cli_len);
                if(fdsock < 0){
                    perror("accept()");
                    close(fdlisten);
                    exit(EXIT_FAILURE);
                }
                    
                // Tworzenie wątku dla gniazda połączonego:
                pthread_t tid;
                pthread_attr_t attr;
                
                // Ustalenie argumentów procedury wątku:
                pthread_attr_init(&attr);

                ret = pthread_create(&tid, &attr, thread_procedure, (void*) &fdsock);
                if(ret != 0){
                    perror("pthread_create()");
                    close(fdlisten);
                    exit(EXIT_FAILURE);
                }
                
            }
        
    }
    
    close(fdlisten);
    cout << "Koniec" << endl;
    
    exit(EXIT_SUCCESS);
}

void *thread_procedure(void *argv){
    int fdsock = *(int*) argv;
    char buffer[SIZE];
    int ret;
    const char *headers, *content;
    char client_buffor[HTML_SIZE];
    
    // Odebranie rządania HTTP:
    ret = recv(fdsock, buffer, sizeof(buffer), 0);
    if(ret < 0){
        perror("recv()");
        close(fdsock);
        return NULL;
    }
    buffer[ret] = '\0';
    
    // Wysłanie odpowiedzi:
    if(strstr(buffer, ".jpg") || strstr(buffer, ".jpeg") || strstr(buffer, ".png") || strstr(buffer, ".gif")){
        
        memset((void*)client_buffor, 0, sizeof(client_buffor));
        char filename[1024];
        getFilename(filename, buffer);
        
        cout << filename << endl;
        
        const char *mime;
        fpos_t filesize;
        
        headers =   "HTTP/1.1 200 OK\n"
                    "Content-Type: %s\n"
                    "Content-Length: %d\n\r\n";
        
        // Rodzaj pliku:
        if(strstr(buffer, ".jpg") || strstr(buffer, ".jpeg"))
            mime = "image/jpeg";
        else if(strstr(buffer, ".png"))
            mime = "image/png";
        else if(strstr(buffer, ".gif"))
            mime = "image/gif";
		else
			mime = "emptyStack";
        
        // Rozmiar pliku:
        FILE * fd = fopen(filename, "rb");
        if(fd){
            fseek(fd, 0, SEEK_END);
            fgetpos(fd, &filesize);
            fseek(fd, 0, SEEK_SET);
        }
        
        sprintf(client_buffor, headers, mime, filename, filesize);
        cout << client_buffor << endl;
        
        if(fd){
            
             // Wyślij nagłówek:
            ret = send(fdsock, client_buffor, strlen(client_buffor), 0);
            if(ret < 0){
                perror("send()");
                close(fdsock);
                return NULL;
            }
            
            // Blokowe wysyłanie pliku (1024B):
            while((ret = read(fileno(fd), client_buffor, SIZE)) > 0){
                ret = send(fdsock, client_buffor, ret, 0);
                if(ret < 0){
                    perror("send()");
                    close(fdsock);
                    return NULL;
                }
            }
            fclose(fd);
        }
    }
    else {
        // Nagłówki:
        headers =   "HTTP/1.1 200 OK\n"
                    "Content-Type: text/html; charset=utf-8\n"
                    "Content-Length: %d\n\r\n";
        
        content = html;
        
        sprintf(client_buffor, headers, strlen(content));
        sprintf(client_buffor, "%s%s", client_buffor, content);
        
        ret = send(fdsock, client_buffor, strlen(client_buffor), 0);
        if(ret < 0){
            perror("send()");
            close(fdsock);
            return NULL;
        }
    }
    close(fdsock);
    
    cout << buffer << endl;
    return NULL;
}

void *create_html(char* html){
    const char *cont_b = "<html>\n<head>\n\t<title>PUS lab 1</title>\n</head>\n<body>\n<center>\n<h1>PUS lab numer 1!!!</h1>\n<div style=\" border: solid 1px #000;\">";
    const char *cont_e = "\n</div>\n</center>\n</body>\n</html>";
    char tmp[1024];

    dirent *de;
    DIR* dir = opendir("img");
    if(dir == NULL){
        perror("opendir()");
        exit(EXIT_FAILURE);
    }
    strcat(html, cont_b);
    while((de = readdir(dir)) != NULL){
        if(strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0) continue;
        sprintf(tmp, "\n\t<img src=\"img/%s\" alt=\"img\" /><br>", de->d_name);
        strcat(html, tmp);
    }
    strcat(html, cont_e);
    return NULL;
}

char* getFilename(char* fileName, char* fullPath){
    char* start = strstr(fullPath, "GET /");
    if(start != NULL){
        start += strlen("GET /");
        char* end = strstr(start, " ");
        int len = strlen(start)-strlen(end);
        strncpy(fileName, start, len);
        fileName[len] = '\0';
        return fileName;
    }
    return NULL;
}