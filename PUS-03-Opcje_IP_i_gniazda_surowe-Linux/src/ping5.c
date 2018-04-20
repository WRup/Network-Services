#include <netdb.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "checksum.h"

struct addrinfo *rp; //wskaznik uzywany do poruszania sie po elementach listy
int socketDescriptor; // deskryptor gniazda
const size_t dataSize=32; //32B

void parentProcess()
{

	int j; //zmienna iteracyjna
	size_t i;//zmienna iteracyjna + numer sekwencji
	char character;//zmienna uzywana do losowania liter do pola danych

	//bufor na naglowek icmp + dane
	unsigned char datagram[sizeof(struct icmphdr) + dataSize];
    struct icmphdr *icmp_header = (struct icmphdr *)datagram;


	printf("Parental process working...\n\n");

	//funkcja wykorzystywana do losowania liter alfabetu
	srand(time(NULL));

    for (i = 1; i <= 4; i++)
    {
    	//losowanie liter alfabetu do datagramu
        for(j = 0; j < dataSize-1; j++)
        {
        	character = (rand() % (91 - 65) + 65);
            datagram[sizeof(icmp_header) + j]=character;
        }

        datagram[sizeof(icmp_header) + 31] = '\0';

        //wypelnianie naglowka icmp
        icmp_header->type = ICMP_ECHO;
        icmp_header->code = 0;
        icmp_header->un.echo.id = htons(getpid());
        icmp_header->un.echo.sequence = htons(i);
        icmp_header->checksum = 0;
        icmp_header->checksum = internet_checksum((unsigned short*)datagram, sizeof(datagram));

        //wysylanie wiadomosci
        if (sendto(socketDescriptor, (const char*) icmp_header, sizeof(datagram), 0, rp->ai_addr, rp->ai_addrlen) < 0)
        {
            perror("sendto() ");
            exit(EXIT_FAILURE);
        }
        printf("Packet has been sent \n\n");

        sleep(1);
    }
    exit(EXIT_SUCCESS);


}
void childProcess()
{
	printf("Child process working...\n\n");

	//bufor na naglowek tcp + dane
	unsigned char datagram[sizeof(struct icmphdr) + dataSize];
    struct sockaddr_in addresStruct;

    //zmienne uzywane do wyluskania danych z wiadomosci zwrotnej
    struct ip *ipheader = (struct ip*) datagram;
    struct icmphdr *icmp_header = (struct icmphdr *) (datagram + sizeof(struct ip));

    socklen_t addresStructSize = sizeof(addresStruct);

    for(int i=0;i<4;i++)
    {
    	//odbieranie icmp zwrotnego
        recvfrom(socketDescriptor, datagram, sizeof(datagram), 0, (struct sockaddr *) &addresStruct, &addresStructSize);

        printf("------IP details------\n");
        printf("Source address: %s\n",inet_ntoa(ipheader->ip_src));
        printf("TTL: %d\n",ipheader->ip_ttl);
        printf("Header length: %d\n",ipheader->ip_hl);
        printf("Destination address: %s\n", inet_ntoa(ipheader->ip_dst));
        printf("------ICMP details------\n");
        printf("Type: %d\n", (int)icmp_header->type);
        printf("Code: %d\n", (int)icmp_header->code);
        printf("ID: %d\n", icmp_header->un.echo.id);
        printf("Sequence number: %d\n", icmp_header->un.echo.sequence);
        printf("\n");
    }


	exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{

    int ttl=10;	//time to live
    struct addrinfo hints; //Struktura zawierajaca wskazowki dla funkcji getaddrinfo()
 	struct addrinfo *result; //Wskaznik na liste zwracana przez getaddrinfo()

    if(argc!=2) //sprawdzenie poprawnosci wywolania
    {
       	perror("Propper invocation: ./ping <targetHostName/IpAdrress>\n");
        exit(EXIT_FAILURE);
    }

    //Wskazowki dla getaddrinfo()
	memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    if(getaddrinfo(argv[1], NULL, &hints, &result)!=0)
    {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    //Przechodzimy kolejno przez elementy listy
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
    	//utworzenie gniazda surowego protokolu icmp 
        if ((socketDescriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        {
            perror("socket()");
            continue;
        }

        //ustawienie opcji ttl
        if (setsockopt(socketDescriptor,IPPROTO_IP, IP_TTL, &ttl, sizeof(int)) == -1)
        {
            perror("setsockopt()");
            exit(EXIT_FAILURE);
        }
        else
            break;
    }

    if (rp == NULL)
    {
        perror( "Client failure: could not create socket.\n");
        exit(EXIT_FAILURE);
    }


    int pid = fork();

    //rozdzielenie na procesy
    if(pid == 0)
    {
    	childProcess();
    }
    else if(pid > 0)
    {
    	parentProcess();
    }
    else 
    {
    	perror("Fork()");
    	exit(EXIT_FAILURE);
    }
}
