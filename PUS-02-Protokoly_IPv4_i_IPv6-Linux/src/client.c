#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> // socket()
#include <netdb.h> // struct addrinfo
#include <sys/types.h>
#include <unistd.h>     // close()
#include <string.h>
#include <errno.h>

#define BUFF_SIZE 256

int main(int argc, char** argv) 
{
    int sockfd;						// Desktryptor gniazda
    int retval;						// Wartosc zwracana przez funkcj

    /* Struktura zawierajaca opcje dla funkcji getaddrinfo(): */
    struct addrinfo hints;				
    /*
     * Wskaznik na liste zwracana przez getaddrinfo() oraz wskaznik uzywany do
     * poruszania sie po elementach listy:
     */
    struct addrinfo *result, *rp;	
	
    /* Struktura sockaddr_storage – niezalezna od wersji protokolu IP */
    struct sockaddr_storage peer_addr;	  

    socklen_t peer_addr_len;		 // Rozmiar struktury w bajtach
    char message[BUFF_SIZE];         // Bufor dla funkcji read()

    if (argc != 3) 
	{
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    memset(&hints, 0, sizeof(hints));				// Wyzerowanie struktury adresowej dla adresu zdalnego (serwera)
    hints.ai_family      = AF_UNSPEC;				// Domena komunikacyjna (rodzina protokolow IPv4 lub IPv6)
    hints.ai_socktype       = SOCK_STREAM; 			// Pakiety TCP
    hints.ai_flags 	 = 0;
    hints.ai_protocol 	 = IPPROTO_TCP;          	// Protokol TCP

    /* Wywolanie funkcji getaddrinfo, ktora zwraca sturkture addrinfo */
    retval = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retval != 0) 
	{
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) 
	{

		// Utworzenie odpowiedniego gniazda w zaleznosci czy argumentem poprzedniej funkcji byl adres z rodziny AF_INET6 czy AF_INET. */
        sockfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
        if (sockfd == -1)
        {
       		perror("socket()");
        	exit(EXIT_FAILURE);
		}
		// Nawiazanie polaczenia (utworzenie asocjacji, askojarzenie adresu zdalnego z gniazdem)
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == -1)
        {
        	perror("connect()");
        	exit(EXIT_FAILURE);
    	}
		
		// Bufory na adres i port hosta
		char ip_addr[NI_MAXHOST], port[NI_MAXSERV];

    	/* Odebranie danych */
		
		// Wyzerowanie bufora do odebrania wiadomosci
    	memset(message, 0, 256);
		// Funkcja getscockname, ktora zwraca adres do ktorego przypisany jest dekstyptor sockfd do bufora peer_addr
		if (getsockname(sockfd,(struct sockaddr *) &peer_addr, &peer_addr_len) == -1) 
		{
      		perror("getsockname() failed");
      		return -1;
		}
		// Wyciagniecie z bufora peer_addr adresu IP oraz portu
		retval = getnameinfo((struct sockaddr *)&peer_addr, peer_addr_len, ip_addr, NI_MAXHOST, port, NI_MAXSERV, NI_NUMERICHOST);
		// Jezeli funkcja getnameinfo zwrocila 0 (tzn. EOK) to wypisujemy adres i port */
		if(retval==0)
		{
				printf("IP Address = %s, Port = %s\n", ip_addr, port);
		}
		// Odebranie wiadomosci
		recv(sockfd, message, 256, 0);
		fprintf(stdout, "DANE OD SERWERA: %s \n", message);
		sleep(1);

    	// Zamkniecie polaczenia TCP
    	fprintf(stdout, "KONIEC PROGRAMU\n");
    	close(sockfd);
	}
    exit(EXIT_SUCCESS);
}
