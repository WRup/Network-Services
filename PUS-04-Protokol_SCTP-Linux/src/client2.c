#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/sctp.h>
#include <time.h>

#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     sockfd;
    int                     retval, bytes;
    char                    *retptr;
    struct addrinfo hints, *result;
    char                    buff[BUFF_SIZE];

    int 					slen, i, flags;
    struct sctp_initmsg     initmsg;
    struct sctp_status      s_status;
    struct sctp_sndrcvinfo  s_sndrcvinfo;
	struct sctp_event_subscribe s_events;

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IP ADDRESS> <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_flags          = 0;
    hints.ai_protocol       = 0;

    retval = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    if (result == NULL) {
        fprintf(stderr, "Could not connect!\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(result->ai_family, result->ai_socktype, IPPROTO_SCTP);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Definiowanie strumieni
    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams = 3;
    initmsg.sinit_max_instreams = 4;
    initmsg.sinit_max_attempts = 5;

    retval = setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof (initmsg));

    if (retval != 0)
    {
        perror("setsockopt()");;
        exit(EXIT_FAILURE);
	}

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);


	memset (&s_events, 0, sizeof (s_events));
    s_events.sctp_data_io_event = 1;
    retval = setsockopt (sockfd, IPPROTO_SCTP, SCTP_EVENTS,(const void *) &s_events, sizeof (s_events));

    // Wypisanie informacji zwiazanych z asocjacja
    slen = sizeof(s_status);
    retval = getsockopt(sockfd, IPPROTO_SCTP, SCTP_STATUS,(void *) &s_status, (socklen_t *) & slen);

    printf ("Id asocjacji: %d\n", s_status.sstat_assoc_id);
    printf ("Stan asocjacji: %d\n", s_status.sstat_state);
    printf ("Liczba strumieni wychodzacych: %d\n", s_status.sstat_outstrms);
	printf ("Liczba strumieni przychodzacych: %d\n", s_status.sstat_instrms);

	// Odebranie danych od serwera
    for (i = 0; i < s_status.sstat_instrms; i++) 
    {
        memset(buff, 0, sizeof(buff));
        retval = sctp_recvmsg(sockfd, (void *) buff, BUFF_SIZE,(struct sockaddr *) NULL, 0, &s_sndrcvinfo, &flags);

        if (retval > 0)
	    {
            buff[retval] = 0;
            printf ("(Nr strumienia: %d) %s\n", s_sndrcvinfo.sinfo_stream, buff);
            
        }
    }
    close(sockfd);
	exit(EXIT_SUCCESS);
}
