#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <stdbool.h>


#include "libpalindrome.h"

/* Funkcja, która sprawdza czy przesłana wiadomość zawiera litery */
bool isDigit(char *buff, int size)
{
	int i;
	for(i=0;i<size;i++)
	{
		if((int)buff[i] < 48 || (int)buff[i] > 57)
		{
			return false;
		}
	}
	return true;
}



int main(int argc, char** argv) {

    int             sockfd; /* Deskryptor gniazda. */
    int             retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    char            buff[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family          =       AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port            =       htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                 =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(sockfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

	while(1){
	
	client_addr_len = sizeof(client_addr);
	/* Oczekiwanie na dane od klienta: */
	retval = recvfrom(sockfd,buff, sizeof(buff),0,(struct sockaddr*)&client_addr, &client_addr_len);
	fprintf(stderr,"Wypisuje retval  = %d\n", retval);
    if(retval == 0)
		break;
    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }
	/* serwer wypisuje adres i numer portu klienta */
    fprintf(stdout, "\nUDP datagram received from %s:%d. Echoing message...\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),ntohs(client_addr.sin_port));

    /* Sprawdzanie czy wiadomosc jest palindromem, czy jest liczba i wysyłanie odpowiedzi: */
	if(atoi(buff) != 0 && isDigit(buff,retval) && is_palindrome(buff, retval)){
		memcpy((void*)buff, (void *)"To jest palindrom.", sizeof(buff));
		retval = sendto(sockfd,buff, sizeof(buff),0,(struct sockaddr*)&client_addr, client_addr_len);
		if (retval == -1) {
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
	}
	/*wysłanie odpowiedzi w przypadku gdy nie jest to palindrome */
	else {
		memcpy((void*)buff, (void *)"To nie jest palindrom.", sizeof(buff));
		retval = sendto(sockfd, buff, sizeof(buff),0,(struct sockaddr*)&client_addr, client_addr_len);
		if (retval == -1) {
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
	}
	}
	
	fprintf(stderr, "\nKoniec");
    close(sockfd);

    exit(EXIT_SUCCESS);
}
