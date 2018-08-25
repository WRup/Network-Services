#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#include <fcntl.h>
#include "libcommon.h"

#define PASS_LEN 128
#define BUF_SIZE 1024

int authenticate_user(LIBSSH2_SESSION *session, struct connection_data *cd, const char* pubkey, const char* privkey, const char* passphrase) {

    int err;
    char *auth_list;
    char password[PASS_LEN];

    auth_list = libssh2_userauth_list(session, cd->username, strlen(cd->username));
    if(auth_list == NULL) 
	{
        if(libssh2_userauth_authenticated(session)) 
		{
            fprintf(stderr, "Authentication succeeded!\n");
            return -1;
        }else 
		{
            print_ssh_error(session, "libssh2_userauth_list()");
            return -1;
        }
    }

    if (strstr(auth_list, "publickey") != NULL) 
	{
        err = libssh2_userauth_publickey_fromfile(session, cd->username, pubkey, privkey, passphrase);
        if (err == 0) 
		{
            return 0;
        }
    }

    if (strstr(auth_list, "password") == NULL) {
        fprintf(stderr, "Password method is not supported\n");
        return -1;
    }

    for(;;){

        if(get_password("Password: ", password, PASS_LEN)) 
		{
            fprintf(stderr, "get_password()\n");
            return -1;
        }

        if(libssh2_userauth_password(session, cd->username, password) == 0) 
		{
            fprintf(stdout, "Authentication succeeded\n");
            memset(password, 0, PASS_LEN);
            break;
        }else 
		{
            fprintf(stdout, "Authentication failed\n");
        }
    }
    return 0;
}

int main(int argc, char **argv) {

    int err;
    int sockfd;
    char dir[BUF_SIZE];
    char filename[BUF_SIZE];
    char mem[BUF_SIZE];
    int bytes;

    struct connection_data *cd;
    LIBSSH2_SESSION *session;
    LIBSSH2_SFTP *sftp;
    LIBSSH2_SFTP_HANDLE *handle;
    FILE *getfile;

    cd = parse_connection_data(argc, argv, CD_ADDRESS | CD_USERNAME);
    if(cd == NULL) 
	{
        exit(EXIT_FAILURE);
    }

    sockfd = establish_tcp_connection(cd);
    if(sockfd == -1) 
	{
        exit(EXIT_FAILURE);
    }
	
    session = libssh2_session_init();
    if(session == NULL) 
	{
        fprintf(stderr, "libssh2_session_init()\n");
        exit(EXIT_FAILURE);
    }

    err = libssh2_session_startup(session, sockfd);
    if(err < 0) 
	{
        print_ssh_error(session, "libssh2_session_startup()");
        exit(EXIT_FAILURE);
    }

    err = authenticate_server(session);
    if(err < 0) 
	{
        exit(EXIT_FAILURE);
    }

    err = authenticate_user(session, cd, "public.key", "private.key", "passphrase");
    if(err < 0) 
	{
        exit(EXIT_FAILURE);
    }

    sftp = libssh2_sftp_init(session);
    if(sftp == NULL) 
	{
        fprintf(stderr, "libssh2_sftp_init()\n");
        exit(EXIT_FAILURE);
    }

    strcpy(dir, "/home/");
    strcat(dir, cd->username);

    handle = libssh2_sftp_opendir(sftp,  dir);
    if(handle == NULL) 
	{
        print_sftp_error(session, sftp, "libssh2_sftp_opendir()");
        exit(EXIT_FAILURE);
    }

    do {
        bytes = libssh2_sftp_readdir_ex(handle, filename, BUF_SIZE, mem,  BUF_SIZE,  NULL);

        if(strlen(mem) > 0) 
		{
            fprintf(stdout, "%s\n", mem);
        }else 
		{
            fprintf(stdout, "%s\n", filename);
        }
		
    }while(bytes > 0);

    if(bytes < 0)
	{
        print_sftp_error(session, sftp, "libssh2_sftp_readdir_ex()");
        exit(EXIT_FAILURE);
    }
	
    err = libssh2_sftp_closedir(handle);
    if (err < 0) {
        print_sftp_error(session, sftp, "libssh2_sftp_closedir()");
        exit(EXIT_FAILURE);
    }

    printf("Podaj nazwe pliku: ");
	scanf("%s", filename);
	strncat(dir,"/",1);
    strncat(dir, filename,sizeof(filename));

	getfile = fopen(filename, "w+");
	if(!getfile)
       printf("Blad! Nie mozna utworzyc pliku\n");

	handle = libssh2_sftp_open(sftp, dir, LIBSSH2_FXF_READ, 0);

	if(!handle) 
	{
        print_sftp_error(session, sftp, "libssh2_sftp_open()");
        exit(EXIT_FAILURE);
	}else
	{
	    fprintf(stdout, "Pobieram plik %s\n", dir);
		while(1)
		{
			bytes = libssh2_sftp_read(handle, mem, sizeof(mem));
			if(bytes > 0)
			{
                fwrite(mem, bytes, 1, getfile);
                fprintf(stdout, "Trwa zapis do pliku!\n");
			}else
                break;
		}
	}

	if(getfile)
        if(fclose(getfile) == 0)
			fprintf(stdout, "Plik pobrany i zapisany\n");

	libssh2_sftp_close(handle);

    err = libssh2_sftp_shutdown(sftp);
    if(err < 0) 
	{
        print_sftp_error(session, sftp, "libssh2_sftp_shutdown()");
        exit(EXIT_FAILURE);
    }

    err = libssh2_session_disconnect(session,  "Shutdown");
    if(err < 0) 
	{
        print_ssh_error(session, "libssh2_session_disconnect()");
        exit(EXIT_FAILURE);
    }

    err = libssh2_session_free(session);
    if(err < 0) 
	{
        print_ssh_error(session, "libssh2_session_free()");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    free_connection_data(cd);
    exit(EXIT_SUCCESS);
}
