#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h> /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int             sockfd;                 	/* Desktryptor gniazda. */
    int             retval;                 	/* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in6 remote_addr;	/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               	/* Rozmiar struktury w bajtach. */
    char            messsage[BUFF_SIZE];       /* Bufor dla funkcji read(). */
    unsigned int 	scope_id;

    if (argc != 4) 
    {
        fprintf(stderr, "Invocation: %s <IPv6 ADDRESS> <PORT> <INTERFACE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP na IPv6: */
    sockfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == -1) 
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
	
    /* Odwzorowanie nazwy interfejsu na indeks */
    scope_id = if_nametoindex(argv[3]);
    if (scope_id == 0)
	{
         printf("if_nametoindex() failed with errno =  %d %s \n", errno,strerror(errno));
	}

    memset(&remote_addr, 0, sizeof(remote_addr)); 					/* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    remote_addr.sin6_family      = AF_INET6; 						/* Domena komunikacyjna (rodzina protokolow IPv6): */
    remote_addr.sin6_port        = htons(atoi(argv[2])); 			/* Numer portu. */
    remote_addr.sin6_scope_id    = scope_id; 						/* indeks interfejsu */
    inet_pton(AF_INET6, argv[1], &remote_addr.sin6_addr); 			/*Konwertuje stringa na sieciowy adres struktury w rodzinie adresow IPv6 */ 
    addr_len                     = sizeof(remote_addr); 			/* Rozmiar struktury adresowej w bajtach. */

    /* Nawiazanie polaczenia (utworzenie asocjacji,skojarzenie adresu zdalnego z gniazdem): */
    if (connect(sockfd, (const struct sockaddr*) &remote_addr, addr_len) == -1) 
	{
        perror("connect()");
        exit(EXIT_FAILURE);
    }
	
    /* Odebranie danych: */
    /* Wyzerowanie bufora: */
    memset(messsage, 0, sizeof(messsage));
    /* Oczekiwanie na odpowiedz i zapisanie jej do bufora "messsage" */
    recv(sockfd, messsage, sizeof(messsage), 0);
    fprintf(stdout, "Received msg: %s \n", messsage);
    sleep(1);

    /* Zamkniecie polaczenia TCP: */
    fprintf(stdout, "Closing connection...\n");
    close(sockfd);

    exit(EXIT_SUCCESS);
}
