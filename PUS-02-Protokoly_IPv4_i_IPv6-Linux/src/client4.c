#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h>     
#include <string.h>
#include <time.h>
#include <errno.h>

int main(int argc, char** argv) 
{
	int             	sockfd;                 /* Desktryptor gniazda. */
	int             	retval;                 /* Wartosc zwracana przez funkcje. */
    struct sockaddr_in 	remote_addr;			/* Gniazdowa struktura adresowa. */
    socklen_t       	addr_len;               /* Rozmiar struktury w bajtach. */
    char            	buff[256];              /* Bufor dla funkcji read(). */


    if (argc != 3) 
    {
    	fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
    	exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) 
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    memset(&remote_addr, 0, sizeof(remote_addr));

    
    remote_addr.sin_family = AF_INET;				/* Domena komunikacyjna (rodzina protokolow): */
    remote_addr.sin_port = htons(atoi(argv[2])); 	/* Numer portu. */
    addr_len = sizeof(remote_addr);					/* Rozmiar struktury adresowej w bajtach. */

    /* Konwersja adresu IP z postaci czytelnej dla czlowieka: */
    retval = inet_pton(AF_INET, argv[1], &remote_addr.sin_addr);
    if (retval == 0) 
    {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } 
    else if (retval == -1) 
    {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    /* Nawiazanie polaczenia (utworzenie asocjacji,
     * skojarzenie adresu zdalnego z gniazdem): */
    if (connect(sockfd, (const struct sockaddr*) &remote_addr, addr_len) == -1) 
    {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Client connected, waiting for server response...\n");

    memset(buff, 0, 256);

    /* Odebranie danych: */
    retval = read(sockfd, buff, sizeof(buff));
    if(retval <= 0)
    {
        perror("read()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Received server response: %s\n", buff);

    /* Zamkniecie polaczenia TCP: */
    fprintf(stdout, "Closing socket...\n");
    close(sockfd);

    exit(EXIT_SUCCESS);    
}