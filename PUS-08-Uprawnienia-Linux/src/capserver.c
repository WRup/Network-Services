#define _GNU_SOURCE     /* getresgid() */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <sys/capability.h>


int main(int argc, char** argv) {

    /* Deskryptory dla gniazda nasluchujacego i polaczonego: */
    int             listenfd, connfd;

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];

    unsigned int    port_number;

    uid_t           ruid, euid, suid; /* Identyfikatory uzytkownika. */
    
    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    port_number = atoi(argv[1]);
/* ZAD 5 */

	/* Tablica uprawnien */
    const cap_value_t cap_vector[1] = { CAP_NET_BIND_SERVICE };
	/* Kopia robocza posiadajaca wszystkie uprawnienia wyzerowane */
    cap_t privilege_dropped = cap_init();
	/* Duplikat kopii roboczej */
    cap_t privilege_off = cap_dup(privilege_dropped);
	/* Powiazanie gniazda tylko z numerem portu < 1024 */ 
    if (port_number < 1024) //zmiany tylko dla portów <1024
    {
	  /* Kopia „privilege_off” - uprawnienie CAP_NET_BIND_SERVICE tylko w zbiorze dozwolonym. Zbiór efektywny i dziedziczny są puste. */
      if (cap_set_flag(privilege_off, CAP_PERMITTED, 1, cap_vector, CAP_SET) == -1) 
      {
      	perror("cap_set_flag()");
      	exit(EXIT_FAILURE);
      }
 	  /* Duplikat kopii roboczej "privilege_off" */
      cap_t privilege_on = cap_dup(privilege_off); 
      /* privilege_on posiada uprawnienie CAP_NET_BIND_SERVICE w zbiorze dozwolonym oraz w zbiorze efektywnym */
      if (cap_set_flag(privilege_on, CAP_EFFECTIVE, 1, cap_vector, CAP_SET) == -1) 
      {
      	perror("cap_set_flag()");
      	exit(EXIT_FAILURE);
      }
 	  /* Przypisanie wątkowi uprawnień z kopii „privilege_on” */
      if (cap_set_proc(privilege_on) == -1) 
      {
      	perror("cap_set_proc()");
      	exit(EXIT_FAILURE);
      }       
    }

/* ZAD 5 */


    /* Utworzenie gniazda dla protokolu TCP: */
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
    server_addr.sin_port            =       htons(port_number);
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                 =       sizeof(server_addr);



    /* Pobranie identyfikatorow uzytkownika (real, effective, save set). */
    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresgid()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "UID: %u, EUID: %u, SUID: %u\n", ruid, euid, suid);



    fprintf(stdout, "Binding to port %u...\n", port_number);
    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(listenfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }
/* ZAD 5 */
	
	/* Przypisanie wątkowi uprawnień z kopii „privilege_off”. */
    if (cap_set_proc(privilege_off) ==-1)
    
	/* Wykonanie operacji wymagającej CAP_NET_BIND_SERVICE nie będzie mozliwe, ponieważ CAP_NET_BIND_SERVICE zostanie usunięte ze zbioru efektywnego */
    {
        perror("cap_set_proc()");
        exit(EXIT_FAILURE);
    }
	/* Porzucenie wszystkich uprawnień w zbiorze efektywnym i dozwolonym */
    if (cap_set_proc(privilege_dropped) == -1)
    {
        perror("cap_set_proc()");
	exit(EXIT_FAILURE);
    } 


    /* Zwolnienie zaalokowanej pamieci: */
    if (cap_free(privilege_off) == -1)
    {
      perror("cap_free()");
      exit(EXIT_FAILURE);
    }
    if (cap_free(privilege_dropped) == -1)
    {
      perror("cap_free()");
      exit(EXIT_FAILURE);
    }

/* ZAD 5 */
    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresgid()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "UID: %u, EUID: %u, SUID: %u\n", ruid, euid, suid);

    /* Program powinien wykonywac sie dalej z EUID != 0 i saved set-user-ID != 0. */
    if (euid == 0 || suid == 0) {
        fprintf(stderr, "Run server as unprivileged user!\n");
        exit(EXIT_FAILURE);
    }




    /* Przeksztalcenie gniazda w gniazdo nasluchujace: */
    if (listen(listenfd, 2) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Server is listening for incoming connection...\n");

    /* Funkcja pobiera polaczenie z kolejki polaczen oczekujacych na zaakceptowanie
     * i zwraca deskryptor dla gniazda polaczonego: */
    client_addr_len = sizeof(client_addr);
    connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (connfd == -1) {
        perror("accept()");
        exit(EXIT_FAILURE);
    }

    fprintf(
        stdout, "TCP connection accepted from %s:%d\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
        ntohs(client_addr.sin_port)
    );

    sleep(5);

    fprintf(stdout, "Closing connected socket (sending FIN to client)...\n");
    close(connfd);

    close(listenfd);

    exit(EXIT_SUCCESS);
}
