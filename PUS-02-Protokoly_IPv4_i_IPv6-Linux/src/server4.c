
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h> /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>

#define USR_MAX 1
int main(int argc, char** argv) {

    int             sockfd; 									/* Deskryptor gniazda. */
    int             retval; 									/* Wartosc zwracana przez funkcje. */
    int 	        new_socket;
    struct          sockaddr_in client_addr, server_addr;    	/* Gniazdowe struktury adresowe (dla klienta i serwera): */
    socklen_t       client_addr_len, server_addr_len;     		/* Rozmiar struktur w bajtach: */

    /* Bufor wykorzystywany przez send(): */
    char*           messsage = "Laboratorium PUS";
    /* Bufor na adres w postaci czytelnej dla czlowieka (tekstowej): */
    char            adresIPv4[INET_ADDRSTRLEN];

    if (argc != 2) 
    {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    /* Utworzenie gniazda dla protokolu TCP na IPv4: */
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) 
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));     					/* Wyzerowanie struktury adresowej serwera: */
    server_addr.sin_family          =       AF_INET;    				/* Domena komunikacyjna (rodzina protokolow IPv4): */
    server_addr.sin_port            =       htons(atoi(argv[1]));     	/* Numer portu: */
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);    		/* Adres nieokreslony (dowolny interfejs sieciowy): */
    server_addr_len                  =       sizeof(server_addr);     	/* Rozmiar struktury adresowej serwera w bajtach: */

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    /* Przekazanie adresu struktury do funkcji bind() */
    retval = bind(sockfd, (struct sockaddr*) &server_addr, server_addr_len);
    if ( retval == -1) 
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");
    client_addr_len = sizeof(client_addr);

    /* Oczekiwanie na dane od klienta: */
    if (listen(sockfd, USR_MAX) == -1) 
    { 
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while(1){
	/* Obsuga jednego kielnta na raz i oczekiwanie na akceptacje polaczenia */
        if ((new_socket = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len)) < 0)
		{
		perror("accept");
		exit(EXIT_FAILURE);
		}
		else
		{
			/* Wypisanie adresu i portu klienta */
			fprintf(stdout,"Client's address: %s \nClient's port: %d \n *****\n",inet_ntop(AF_INET, &client_addr.sin_addr, adresIPv4, INET_ADDRSTRLEN ), ntohs(client_addr.sin_port));
			/* Wyslanie wiadomosci do klienta */
			if(send( new_socket, messsage, strlen(messsage), 0) == -1)
			{
			perror("send()");
			exit(EXIT_FAILURE);
			}
		}
        close(new_socket);
    }

    exit(EXIT_SUCCESS);
}
