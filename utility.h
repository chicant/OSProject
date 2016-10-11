
#ifndef UTILITY_H
#define	UTILITY_H

// inclusione delle librerie
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h> // gestione segnali
#include <fcntl.h> // per i file descriptors
#include <ifaddrs.h>


// macros
#define BUF_SIZE 	1024
#define USR_SIZE    31
#define MIT_SIZE    42
#define OBJ_SIZE    111
#define TXT_SIZE    4010
#define MAX		    256
#define STR 		char*
#define SERVER_ADDR "127.0.0.1" // indirizzo del server (locale)
#define SERVER_PORT 3000
#define MAX_USERS	128
#define TERMSTR		'\0'
#define ESCAPE      '\n'
#define SEM_USRS    "/sem_users"
#define SEM_BOARD   "/sem_board"
#define SEM_TEMP    "/sem_temp"
#define SEM_DEL     "/sem_delete"

// messaggi standard server
#define CONN_REF 	"Connessione rifiutata"
#define CONN_ACC	"Connessione accettata"
#define OK          "Ok"
#define KO 		    "Ko"
#define KK          "Kk"
#define NE          "Ne"
#define BOARD		"board.txt"
#define USRS   		"users.txt"
#define TEMP        "temp.txt"
#define ESC         "\n"

// utility di gestione dell'errore
#define ERROR_HELPER(return, err)  do {                     \
    if (return < 0) {                                       \
        fprintf(stderr, "%s: %s\n", err, strerror(errno));  \
        exit(EXIT_FAILURE);                                 \
    }                                                       \
} while (0)

// funzione ausiliaria che crea un semaforo named
sem_t* create_n_sem(const STR name, mode_t mode, unsigned int val) {
    sem_t* s = sem_open(name, O_CREAT | O_EXCL, mode, val);
    if (s == SEM_FAILED && errno == EEXIST) {
        printf("Il semaforo esiste già, lo unlinko e riapro...\n");
        sem_unlink(name);
        s = sem_open(name, O_CREAT | O_EXCL, mode, val);
    }
    if (s == SEM_FAILED) ERROR_HELPER(-1, "Errore nella creazione del semaforo");
    return s;
}

// funzione ausiliaria che svuota una stringa
int restr(STR str, size_t size) {
    int i;
    for (i = 0; i < size; ++i) str[i] = TERMSTR;
    return 0;
}

// funzione ausiliaria che assegna un stringa(macro) ad un buffer di char
int char_macro(STR buffer, size_t b_len, STR macro) {
    int i;
    for (i = 0; i < b_len; ++i) buffer[i] = macro[i];
    return 0;
}

#endif	/* UTILITY_H */
