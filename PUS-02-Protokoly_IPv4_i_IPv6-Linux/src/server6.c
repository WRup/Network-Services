#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h>     
#include <string.h>
#include <time.h>
#include <errno.h>

#define USER_MAX 10

int main(int argc, char** argv) 
{
	 /* Deskryptory dla gniazda nasluchujacego i polaczonego: */
    int             listenfd, connfd;
    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in6 client_addr, server_addr;
    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;
    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            ipClientString[256];
    /* Bufor na wiadomosc dla klienta*/
    char* 			message;

    if (argc != 2) 
    {
	    fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
	    exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    listenfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd == -1) 
    {
        perror("socket()\n");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin6_family          =       AF_INET6;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin6_addr		     =       in6addr_any;
    /* Numer portu: */
    server_addr.sin6_port            =       htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                  =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(listenfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()\n");
        exit(EXIT_FAILURE);
    }

    /* Przeksztalcenie gniazda w gniazdo nasluchujace: */
    if (listen(listenfd, USER_MAX) == -1) {
        perror("listen()\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

	/* Rozmiar struktury adresowej klienta w bajtach: */
    client_addr_len = sizeof(client_addr);

    while(1)
    {
    	/* Odebraie polaczenia klienta*/
    	connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);

    	if (connfd == -1) 
    	{
        	perror("accept()\n");
        	exit(EXIT_FAILURE);
    	}
    	else
    	{
    		/* Wypisanie klienta */
		    fprintf(
	        	stdout, "TCP connection accepted from %s:%d\n",
	        	inet_ntop(AF_INET6, &client_addr.sin6_addr, ipClientString, sizeof(ipClientString)),
	        	ntohs(client_addr.sin6_port)
	    	);

		    /* Sprawdzenei wersji protokolu IP */
		    if(IN6_IS_ADDR_V4MAPPED(&client_addr.sin6_addr))
			{
				fprintf(stdout,"Client: IP ver. 4 \n");
			}
			else
			{
				fprintf(stdout,"Client: IP ver. 6\n");
			}

			/* Wyslanie wiadomosci do klienta */
			message = (char *) "Laboratorium PUS";
			if(write(connfd, message, strlen(message)) <= 0)
			{
				perror("write()\n");
        		exit(EXIT_FAILURE);
			}

			/* Konczenie obslugi klienta */
			fprintf(stdout, "Closing client socket ...\n");
    		close(connfd);
    	}



    }
}