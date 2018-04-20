#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>

#define MAX 256			/* wielkość bufora wiadomości */

int main(int argc, char** argv) {

    int             sockfd;                 /* Desktryptor gniazda. */
    int             retval;                 /* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in remote_addr;/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               /* Rozmiar struktury w bajtach. */
    char            recvline[MAX];          /* Bufor dla funkcji recvfrom(). */
	char			sendline[MAX];			/* Bufor do pobrania danych ze standardowego wejscia. */
	char 			fGetter;				/* Bufor na pojedyncze znaki wiadomosci. */
	int 			i;						/* Iterator */


    if (argc != 3) {
        fprintf(stderr,"Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    memset(&remote_addr, 0, sizeof(remote_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    remote_addr.sin_family = AF_INET;

    /* Konwersja adresu IP z postaci kropkowo-dziesietnej: */
    retval = inet_pton(AF_INET, argv[1], &remote_addr.sin_addr);
    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_port = htons(atoi(argv[2])); /* Numer portu. */
    addr_len = sizeof(remote_addr); /* Rozmiar struktury adresowej w bajtach. */
	
	/* Nawiazanie polaczenia (skojarzenie adresu zdalnego z gniazdem): */
	retval = connect(sockfd, (const struct sockaddr*)&remote_addr, addr_len);
    if(retval == -1){
        perror("connect()");
        exit(EXIT_FAILURE);
    }
	while(1){
	/* pobranie danych do wysłania ze standardowego wejścia */
	i=0;
	while((fGetter = getchar()) != '\n' && i < MAX){
        sendline[i] = fGetter;
        ++i;
    }
	/* send() wysyla dane na adres okreslony przez strukture 'remote_addr': */
    retval = send(sockfd,sendline, i,0);
    if (retval == -1) {
        perror("send()");
        exit(EXIT_FAILURE);
    }
	if(i == 0){
		fprintf(stderr, "Koncze program.");
		break;
	}
	fprintf(stdout,"Sending message to %s.\nWaiting for server response...\n", argv[1]);
    /* Oczekiwanie na odpowiedz i zapisanie jej do bufora recvline */
    retval = recv(sockfd, recvline, sizeof(recvline), 0);
    if (retval == -1) {
        perror("recv()");
        exit(EXIT_FAILURE);
    }
	
	/* wypisanie odebranej wiadomosci */
    fprintf(stdout, "Server response: '%s' \n", recvline);
	}
	

    close(sockfd);

    exit(EXIT_SUCCESS);
}
