/*
 * Kompilacja:          $ gcc server4.c -o server4
 * Uruchamianie:        $ ./server4 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>

#include <errno.h>

#define USR_MAX 10      // Maksymalna liczba polaczonych uzytkownikow

int main(int argc, char** argv) {

    int             retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor na dane przychodzace od klienta */
    char            buf[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    //char            addr_buf[256];

    fd_set master;      // główna lista deskryptorów plików
    fd_set read_fds;    // pomocnicza lista deskryptorów dla select()
    int fdmax;          // najwyzszy numer deskryptora pliku
    int listenfd;       // deskryptor gniazda nasluchujacego
    int newfd;          // deskryptor gniazda nowozaakceptowanego
    int i,j;            // iteratory

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Czyscimy listy deskryptorow
    FD_ZERO( & master ); 
    FD_ZERO( & read_fds );

    /* Utworzenie gniazda nasluchujacego: */
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
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
    if (bind(listenfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // Przypisujemy gniazdo do nasluchiwania nowych polaczen
    if( listen( listenfd, USR_MAX  ) == - 1 ) {
        perror( "listen" );
        exit( 1 );
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");
    
    FD_SET( listenfd, & master );// dodajemy gniazdo nasluchujace do glownej listy deskryptorow

    fdmax = listenfd; // ustawiamy najwyzszy numer deskryptora pliku
   
    // pętla główna
	while(1){
		
	    read_fds = master; // kopiujemy liste deskryptorow
        if( select( fdmax + 1, & read_fds, NULL, NULL, NULL ) == - 1 ) {
            perror( "select" );
            exit( 1 );
        }

        //przeszukujemy polaczenia w celu znalezienia danych do odczytu
        for( i = 0; i <= fdmax; i++ ) {
                    if( FD_ISSET( i, & read_fds ) ) { 
                        if( i == listenfd ) {   // nowe polaczenie
                            
                            client_addr_len = sizeof( client_addr );
                            newfd = accept( listenfd,( struct sockaddr * ) & client_addr, & client_addr_len );
                            if(newfd == - 1 ) {
                                perror( "accept" );
                            } 
                            else {
                                FD_SET( newfd, & master ); // dodajemy polaczenie do glownej listy deskryptorow
                                if( newfd > fdmax ) { // sprawdzamy najwyzszy numer deskryptora pliku
                                    fdmax = newfd;
                                }
                                printf( "Select server: new connection from %s on socket %d\n", 
                                    inet_ntoa(client_addr.sin_addr), newfd );

                            }
                        }
                        else {
                            //obslugujemy dane od klienta
                            if(( retval = recv( i, buf, sizeof( buf ), 0 ) ) <= 0 ) {
                                // blad lub połączenie zostało zerwane
                                if( retval == 0 ) {
                                    // polaczenie zerwane
                                    printf( "selectserver: socket %d hung up\n", i );
                                } else {
                                    perror( "recv" );
                                }
                                close( i );
                                FD_CLR( i, & master ); // usuwamy z glownej listy deskryptorow
                            }
                            else {
                                
                                // odebralismy dane od klienta
                                for( j = 0; j <= fdmax; j++ ) {
                                    // wysylanie do wszystkich
                                    if( FD_ISSET( j, & master ) ) {
                                        // oprocz nas i gniazda nasluchujacego
                                        if( j != listenfd && j != i ) {
                                            if( send( j, buf, retval, 0 ) == - 1 ) {
                                                perror( "send" );   
                                            }
                                        }
                                    }
                                }
                            }

                        }
                    }
        }

	}
	
	fprintf(stdout, "Koniec \n");

    exit(EXIT_SUCCESS);
}

