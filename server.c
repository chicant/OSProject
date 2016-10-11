#include "utility.h"

// variabili globali
int usr_desc, socket_desc;
unsigned int curr_usrs;
sem_t* sem_usrs; // semaforo per leggere/scrivere sul file utenti
sem_t* sem_board; // semaforo per leggere/scrivere sul file bacheca
sem_t* sem_temp; // semaforo per leggere/scrivere sul file temporaneo
sem_t* sem_del; // semaforo per sostituire o meno il file bacheca

/**********************************************************************************
*   register_handler(int desc): funzione che gestisce la registrazione            *
*                   dell'utente. Riceve come unico paramentro il descrittore      *
*                   della socket relativa al client correntemente servito.        *
**********************************************************************************/

int register_handler(int desc) {
    int ret, r, msg_len, flag, i;
    char cred[MAX], resp[3], user[USR_SIZE], buf[MAX];

    // mando l'OK al client per la ricezione delle credenziali
    r = sizeof(resp);
    char_macro(resp, r, OK);
    while ((ret = send(desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'ok iniziale in register_handler()");
    }

    // mi metto in attesa delle credenziali
    re_reg:
    restr(cred, sizeof(cred));
    while ((msg_len = recv(desc, cred, sizeof(cred), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive delle credenziali in register_handler()");
    }

    // controllo se sia già presente un altro utente con lo stesso nickname
    strncpy(buf, cred, sizeof(buf));
    STR token = strtok(buf, " ");
    strncpy(user, token, sizeof(user));


    FILE* usrs;
    usrs = fopen(USRS, "r");
    if (usrs == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file utenti in login_handler()");

    sem_wait(sem_usrs);
    restr(buf, sizeof(buf));
    flag = i = 0;
    // cerco fra quelli registrati
    while (fgets(buf, sizeof(buf), usrs) != NULL) {
        for (token = strtok(buf, " "); token != NULL; token = strtok(NULL, "\n"), i++) {
            if (flag == 0 && i%2 == 0) {
                if (strcmp(token, user) == 0) {
                    flag = 1;
                }
            }
        }
        if (flag == 1) break;
    }
    sem_post(sem_usrs);
    fclose(usrs);

    if (flag == 1) {
        // lo username esiste già
        char_macro(resp, r, KK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send del kk finale in register_handler()");
        }
        goto re_reg;
    } else {
        // aggiungo le credenziali al file
        sem_wait(sem_usrs);
        ret = write(usr_desc, cred, strlen(cred));
        if (ret == -1) ERROR_HELPER(ret, "Errore nell'aggiunta delle credenziali nel file utenti "
                                            "in register_handler()");
        sem_post(sem_usrs);
        if (ret > 0) { // se è andato tutto bene mando l'OK
            char_macro(resp, r, OK);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send dell'ok finale in register_handler()");
            }
            return 0;
        } else { // altrimenti mando KO
            char_macro(resp, r, KO);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send del ko finale in register_handler()");
            }
            return -1;
        }
    }
}

/**********************************************************************************
*   login_handler(int desc): funzione che gestisce il login di ogni singolo       *
*                client. Riceve come unico paramentro il descrittore della socket *
*                relativa al client correntemente servito.                        *
**********************************************************************************/

int login_handler(int desc) {
    int ret, msg_len, flag = 0, i = 0, r;
    char buf[BUF_SIZE], resp[3], user[USR_SIZE], password[20], cred[MAX];
    char tuser[USR_SIZE], tpass[20];

    // mando l'ok al client per la ricezione delle credenziali
    r = sizeof(resp);
    char_macro(resp, r, OK);
    while ((ret = send(desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'ok iniziale in login_handler()");
    }

    // mi metto in attesa delle credenziali
    while ((msg_len = recv(desc, buf, sizeof(buf), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive delle credenziali in login_handler()");
    }

    STR token;
    // divido le credenziali
    for (token = strtok(buf, " "); token != NULL; token = strtok(NULL, " -\n-\0")) {
        if (flag == 1) {
            strncpy(password, token, sizeof(password));
            break;
        }
        strncpy(user, token, sizeof(user));
        flag = 1;
    }

    FILE* usrs;
    usrs = fopen(USRS, "r");
    if (usrs == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file utenti in login_handler()");

    sem_wait(sem_usrs);
    flag = 0;
    // cerco fra quelle salvate
    while (fgets(cred, sizeof(cred), usrs) != NULL) {
        for (token = strtok(cred, " "); token != NULL; token = strtok(NULL, "\n"), i++) {
            if (flag == 1) {
                strncpy(tpass, token, sizeof(tpass));
                break;
            } else if (flag == 0 && i%2 == 0) {
                if (strcmp(token, user) == 0) {
                    strncpy(tuser, token, sizeof(tuser));
                    flag = 1;
                }
            }
        }
        if (flag == 1) break;
    }
    sem_post(sem_usrs);
    fclose(usrs);

    if (flag == 0) { // il nome utente non è presente nel file
        char_macro(resp, r, KK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send del messaggio 'utente non presente' finale in"
                                " login_handler()");
        }
        return 0;
    } else {
        // confronto password
        ret = strcmp(tpass, password);
        if (ret == 0) { // se è andato tutto bene mando l'OK
            char_macro(resp, r, OK);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send dell'ok finale in login_handler()");
            }
            return 0;
        } else { // altrimenti mando KO
            char_macro(resp, r, KO);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send del ko finale in login_handler()");
            }
            return 0;
        }
    }
}

/**********************************************************************************
*   read_board_handler(int desc): funzione che gestisce la lettura della bacheca. *
*                     Riceve come unico paramentro il descrittore della socket    *
*                     relativa al client correntemente servito.                   *
**********************************************************************************/

int read_board_handler(int desc) {
    char resp[3], line[MAX], lines[4];
    int ret = 0, count = 0, r = 0;

    // mando l'ok al client per l'avvenuta ricezione del comando
    r = sizeof(resp);
    char_macro(resp, r, OK);
    while ((ret = send(desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'ok iniziale in read_board_handler()");
    }

    // apro il file della bacheca
    sem_wait(sem_del);
    FILE* board;
    board = fopen(BOARD, "r");
    if (board == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file della bacheca in "
                                        "read_board_handler()");

    sem_wait(sem_board);
    // leggo e invio il numero di righe
    while (fgets(line, sizeof(line), board) != NULL) {
        count++;
    }
    sprintf(lines, "%d", count);
    while ((ret = send(desc, lines, sizeof(lines), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del numero di righe in read_board_handler()");
    }

    // mi riposiziono all'inizio del file
    fseek(board, 0L, SEEK_SET);

    // leggo e invio tutta la bacheca
    while (fgets(line, sizeof(line), board) != NULL) {
        while ((ret = send(desc, line, sizeof(line), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send delle righe della bacheca in "
                                "read_board_handler()");
        }
    }
    sem_post(sem_board);
    fclose(board);
    sem_post(sem_del);

    return 0;
}

/**********************************************************************************
*   add_msg_handler(int desc): funzione che gestisce l'aggiunta di un messaggio   *
*                  alla bacheca. Riceve come unico paramentro il descrittore      *
*                  della socket relativa al client correntemente servito.         *
**********************************************************************************/

int add_msg_handler(int desc) {
    char resp[3], usr[USR_SIZE], obj[OBJ_SIZE], txt[TXT_SIZE], riga[TXT_SIZE];
    int ret, msg_len, r, lines, found;

    // mando l'ok al client per l'avvenuta ricezione del comando
    r = sizeof(resp);
    char_macro(resp, r, OK);
    while ((ret = send(desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'ok iniziale in add_msg_handler()");
    }

    // ricevo il mittente
    re_mit:
    while ((msg_len = recv(desc, usr, sizeof(usr), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive del mittente in add_msg_handler()");
    }
    char mitt[MIT_SIZE] = "MITTENTE: ";
    strncat(mitt, usr, sizeof(mitt));
    strncat(mitt, ESC, sizeof(mitt));

    // invio il primo OK/KO
    if (ret > 0) {
        char_macro(resp, r, OK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send dell'OK mittente in add_msg_handler()");
        }
    } else {
        char_macro(resp, r, KO);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send del KO mittente in add_msg_handler()");
        }
        goto re_mit;
    }

    // ricevo l'oggetto
    re_obj:
    restr(obj, sizeof(obj));
    while ((msg_len = recv(desc, obj, sizeof(obj), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'oggetto in add_msg_handler()");
    }
    char ogg[OBJ_SIZE] = "OGGETTO: ";
    strncat(ogg, obj, sizeof(ogg));
    strncat(ogg, ESC, sizeof(ogg));

    // cerco nel file bacheca
    FILE* board;
    board = fopen(BOARD, "r");
    if (board == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file bacheca in "
                                        "add_msg_handler()");

    sem_wait(sem_board);
    lines = found = 0;
    while (fgets(riga, sizeof(riga), board) != NULL) {
        if (strcmp(riga, ESC) == 0) continue;
        if (lines == 1 && strcmp(riga, ogg) == 0) { // oggetto
            found = 1;
            break;
        }
        if (lines == 2) { // testo
            lines = 0;
            continue;
        }
        lines++;
    }
    sem_post(sem_board);
    fclose(board);

    if (found == 1) {
        char_macro(resp, r, KK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send dell'oggetto già presente in add_msg_handler()");
        }
        goto re_obj;
    } else {
        // invio il secondo OK/KO
        if (ret > 0) {
            char_macro(resp, r, OK);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send dell'ok oggetto in add_msg_handler()");
            }
        } else {
            char_macro(resp, r, KO);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send del ko oggetto in add_msg_handler()");
            }
            goto re_obj;
        }

        // ricevo il testo
        while ((msg_len = recv(desc, txt, sizeof(txt), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(msg_len, "Errore nella receive del testo in add_msg_handler()");
        }
        char testo[TXT_SIZE] = "TESTO: ", fine[] = "\n\n";
        strncat(testo, txt, sizeof(testo));
        strncat(testo, fine, sizeof(testo));

        // inserisco il mittente nel file
        sem_wait(sem_del);
        int board_desc = open(BOARD, O_RDWR | O_CREAT | O_APPEND, 0660);
        sem_wait(sem_board);
        ret = write(board_desc, mitt, strlen(mitt));
        if (ret == -1) ERROR_HELPER(ret, "Errore nella scrittura del mittente nel file bacheca "
                        "in add_msg_handler()");

        // inserisco l'oggetto nel file
        ret = write(board_desc, ogg, strlen(ogg));
        if (ret == -1) ERROR_HELPER(ret, "Errore nella scrittura dell'oggetto nel file bacheca"
                        " in add_msg_handler()");

        // inserisco il testo nel file
        ret = write(board_desc, testo, strlen(testo));
        if (ret == -1) ERROR_HELPER(ret, "Errore nella scrittura del testo nel file bacheca in "
                    "add_msg_handler()");
        sem_post(sem_board);
        close(board_desc);
        sem_post(sem_del);

        // invio del terzo e ultimo OK/KO
        if (ret > 0) {
            char_macro(resp, r, OK);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send dell'ok testo/finale in add_msg_handler()");
            }
        } else {
            char_macro(resp, r, KO);
            while ((ret = send(desc, resp, r, 0)) < 0) {
                if (errno == EINTR) continue;
                ERROR_HELPER(ret, "Errore nella send del ko testo/finale in add_msg_handler()");
            }
        }
        return 0;
    }
}

/**********************************************************************************
*   del_msg_handler(int desc): funzione che gestisce la cancellazione di un       *
*                  messaggio dalla bacheca. Cancella il messaggio solo se è       *
*                  l'autore a richiederla. Riceve come unico paramentro il        *
*                  descrittore della socket relativa al client correntemente      *
*                  servito.                                                       *
**********************************************************************************/

int del_msg_handler(int desc) {
    char resp[3], mitt[USR_SIZE], obj[OBJ_SIZE], usr[USR_SIZE], riga[TXT_SIZE];
    int ret, msg_len, i, flag, r, found, auth, lines, count;

    // mando l'ok al client per l'avvenuta ricezione del comando
    r = sizeof(resp);
    char_macro(resp, r, OK);
    while ((ret = send(desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'ok iniziale in del_msg_handler()");
    }

    // ricevo il mittente
    remit:
    while ((msg_len = recv(desc, mitt, sizeof(mitt), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive del mittente in del_msg_handler()");
    }

    // controllo che il mittente sia nel file degli utenti
    FILE* usrs;
    usrs = fopen(USRS, "r");
    if (usrs == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file utenti in del_msg_handler()");

    sem_wait(sem_usrs);
    i = flag = 0;
    STR token;
    while (fgets(usr, sizeof(usr), usrs) != NULL) {
        for (token = strtok(usr, " "); token != NULL; token = strtok(NULL, "\n"), i++) {
            if (i%2 == 0) {
                if (strcmp(token, mitt) == 0) {
                    flag = 1;
                    break;
                }
            }
        }
    }
    sem_post(sem_usrs);
    fclose(usrs);

    // invio il primo OK/KO
    if (flag > 0) {
        char_macro(resp, r, OK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send dell'OK mittente in del_msg_handler()");
        }
    } else {
        char_macro(resp, r, KO);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send del KO mittente in del_msg_handler()");
        }
        goto remit;
    }

    // processo il mittente
    char mittente[MIT_SIZE] = "MITTENTE: ";
    strncat(mittente, mitt, sizeof(mittente));
    strncat(mittente, ESC, sizeof(mittente));

    // ricevo l'oggetto
    while ((msg_len = recv(desc, obj, sizeof(obj), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'oggetto in del_msg_handler()");
    }

    // processo l'oggetto
    char oggetto[OBJ_SIZE] = "OGGETTO: ";
    strncat(oggetto, obj, sizeof(oggetto));
    strncat(oggetto, ESC, sizeof(oggetto));

    // cerco nel file bacheca
    sem_wait(sem_del);
    FILE* board;
    board = fopen(BOARD, "r");
    if (board == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file bacheca in "
                                        "del_msg_handler()");

    // controllo che il messaggio esista, se si ne segno il numero e se
    // l'utente possiede l'autorizzazione
    sem_wait(sem_board);
    lines = found = auth = count = 0;
    while (fgets(riga, sizeof(riga), board) != NULL) {
        if (strcmp(riga, ESC) == 0) continue;
        if (lines == 0) {
            count++;
            if (strcmp(riga, mittente) == 0) auth = 1;
        }
        if (lines == 1 && strcmp(riga, oggetto) == 0) {
            found = 1;
            break;
        }
        if (lines == 2) {
            lines = 0;
            auth = 0;
            continue;
        }
        lines++;
    }
    sem_post(sem_board);
    sem_post(sem_del);
    fclose(board);

    if (found == 1 && auth == 0) { // non si possiede l'autorizzazione
        char_macro(resp, r, KK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send di 'utente non autorizzato' in "
                        "del_msg_handler()");
        }
        return 0;
    } else if (found == 0) { // non esiste
        char_macro(resp, r, NE);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send di 'messaggio non esistente' "
                        "mittente in del_msg_handler()");
        }
        return 0;
    } else { // copio il contenuto del file originale in uno temporaneo omettendo
        // il messaggio da cancellare
        restr(riga, sizeof(riga));

        sem_wait(sem_del);
        board = fopen(BOARD, "r");
        if (board == NULL) ERROR_HELPER(-1, "Errore nell'apertura del file bacheca in "
                                            "del_msg_handler()");
        sem_wait(sem_temp);
        ret = creat(TEMP, 0600);
        if (ret == -1) ERROR_HELPER(ret, "Errore nella creazione del file temp in "
                                            "del_msg_handler()");
        int temp = open(TEMP, O_WRONLY, 0600);
        if (temp == -1) ERROR_HELPER(temp, "Errore nell'apertura del file temp in "
                                            "del_msg_handler()");

        sem_wait(sem_board);
        flag = 1, lines = 0;
        while (fgets(riga, sizeof(riga), board) != NULL) {
            if (flag == count) {
                lines++;
                if (lines == 4) flag = 0;
                continue;
            }
            ret = write(temp, riga, strlen(riga));
            if (ret == -1) ERROR_HELPER(ret, "Errore nella scrittura del file temp in "
                                                "del_msg_handler()");
            lines++;
            if (lines == 4) {
                lines = 0;
                flag++;
            }
        }
        sem_post(sem_board);
        fclose(board);
        close(temp);

        sem_wait(sem_board);
        ret = remove(BOARD);
        if (ret == -1) ERROR_HELPER(ret, "Errore nella rimozione del vecchio file bacheca in "
                                            "del_msg_handler()");
        ret = rename(TEMP, BOARD);
        if (ret == -1) ERROR_HELPER(ret, "Errore nella rename del nuovo file bacheca in "
                                            "del_msg_handler()");
        sem_post(sem_board);
        sem_post(sem_temp);
        sem_post(sem_del);

        // se è andato tutto bene
        char_macro(resp, r, OK);
        while ((ret = send(desc, resp, r, 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send dell'OK finale in del_msg_handler()");
        }
        return 0;
    }
}

/**********************************************************************************
*   conn_close_handler(int desc): funzione che gestisce la chiusura della singola *
*                         connessione con un client.                              *
**********************************************************************************/

int conn_close_handler(int desc) {
    char resp[3];
    int ret, r;
    r = sizeof(resp);

    // mando l'ok finale al client
    char_macro(resp, r, OK);
    while ((ret = send(desc, resp, sizeof(resp), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'ok finale del metodo conn_close()");
    }

    // termino il thread
    pthread_exit(NULL);
    return 0;
}

/**********************************************************************************
*   spipe_handler(int desc): funzione che gestisce la chiusura della singola      *
*                         connessione con un client in caso di segnale SIGPIPE.   *
**********************************************************************************/

void spipe_handler() {
    pthread_exit(NULL);
}

/**********************************************************************************
*   server_close_handler(int desc): funzione che gestisce la chiusura del server  *
*                         in modo sicuro.                                         *
**********************************************************************************/

void server_close_handler() {
    int ret;
    printf("\nChiusura del server...\n");

    // chiudo il descrittore del file utenti
    close(usr_desc);

    // chiudo la socket del server
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Errore nella chiusura della socket del server");

    //chiudo i semafori
    ret = sem_close(sem_usrs);
    ERROR_HELPER(ret, "Errore nella chiusura del semaforo utenti");
    ret = sem_unlink(SEM_USRS);
    ERROR_HELPER(ret, "Errore nella unlink del semaforo utenti");

    ret = sem_close(sem_board);
    ERROR_HELPER(ret, "Errore nella chiusura del semaforo bacheca");
    ret = sem_unlink(SEM_BOARD);
    ERROR_HELPER(ret, "Errore nella unlink del semaforo bacheca");

    ret = sem_close(sem_temp);
    ERROR_HELPER(ret, "Errore nella chiusura del semaforo file temporaneo");
    ret = sem_unlink(SEM_TEMP);
    ERROR_HELPER(ret, "Errore nella unlink del semaforo file temporaneo");

    ret = sem_close(sem_del);
    ERROR_HELPER(ret, "Errore nella chiusura del semaforo sostituzione file bacheca");
    ret = sem_unlink(SEM_DEL);
    ERROR_HELPER(ret, "Errore nella unlink del semaforo sostituzione file bacheca");

    printf("Server chiuso correttamente.\n");
    exit(EXIT_SUCCESS);
}

/**********************************************************************************
*   conn_handler(void* c_desc): funzione che gestisce ogni singola connessione da *
*		             parte di un client, smistando i compiti fra le varie 		  *
*		             sotto-funzioni. Riceve come unico parametro il descrittore   *
*                    del client corrente.                                         *
**********************************************************************************/

void* conn_handler(void* c_desc) {
    int msg_len = 0, tmp = 0;
    char command[2];
    int sock_desc = *((int*)c_desc);

    while (1) {
        //mi metto in attesa del comando dell'utente
        restr(command, sizeof(command));
        while ((msg_len = recv(sock_desc, command, sizeof(command), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(msg_len, "Errore nella ricezione del comando in conn_handler()");
        }
        command[1] = TERMSTR;

        // assegno la giusta funzione per ogni comando
        if (strcmp(command, "0") == 0) {
            tmp = login_handler(sock_desc);
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nel ritorno da login()");
        } else if (strcmp(command, "1") == 0) {
            tmp = read_board_handler(sock_desc);
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nel ritorno da read_board()");
        } else if (strcmp(command, "2") == 0) {
            tmp = add_msg_handler(sock_desc);
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nel ritorno da add_msg_handler()");
        } else if (strcmp(command, "3") == 0) {
            tmp = del_msg_handler(sock_desc);
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nel ritorno da del_msg_handler()");
        } else if (strcmp(command, "5") == 0) {
            tmp = register_handler(sock_desc);
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nel ritorno da register_handler()");
        } else {
            conn_close_handler(sock_desc);
        }
    }
}

/**********************************************************************************
*   funzione main()	 							                                  *
**********************************************************************************/

int main() {
    int client_desc, sockaddr_len, reuseaddr_opt, ret;
    char buf[BUF_SIZE];

    struct sockaddr_in server = {0};
    sockaddr_len = sizeof(struct sockaddr_in);

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Impossibile creare la socket");

    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if (ret < 0) ERROR_HELPER(ret, "Errore nella setsockopt");

    ret = bind(socket_desc, (struct sockaddr*) &server, sizeof(server));
    if (ret < 0) ERROR_HELPER(ret, "Errore nella bind");

    ret = listen(socket_desc, MAX_USERS);
    if (ret < 0) ERROR_HELPER(ret, "Errore nella listen");

    struct sockaddr_in* client = calloc(1, sizeof(struct sockaddr_in));

    // prendo tutti gli indirizzi del server e li stampo
    struct ifaddrs* server_infos = {0};
    getifaddrs(&server_infos);
    struct ifaddrs* temp = server_infos;
    printf("\nIndirizzi per connettersi al server:\n\n");
    while (temp) {
        if (temp->ifa_addr && temp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)temp->ifa_addr;
            printf("%s: %s\n\n", temp->ifa_name, inet_ntoa(addr->sin_addr));
        }
        temp = temp->ifa_next;
    }
    freeifaddrs(server_infos);

    // apro il file degli utenti
    usr_desc = open(USRS, O_RDWR | O_CREAT | O_APPEND, 0660);
    if (usr_desc == -1) ERROR_HELPER(usr_desc, "Errore nell'apertura del file utenti nel main()");

    // inizializzazione dei semafori
    sem_usrs = create_n_sem(SEM_USRS, 0660, 1);
    sem_board = create_n_sem(SEM_BOARD, 0660, 1);
    sem_temp = create_n_sem(SEM_TEMP, 0660, 1);
    sem_del = create_n_sem(SEM_DEL, 0660, 1);

    // assegna la ruotine ai vari segnali
    signal(SIGHUP, server_close_handler);
    signal(SIGINT, server_close_handler);
    signal(SIGQUIT, server_close_handler);
    signal(SIGILL, server_close_handler);
    signal(SIGSEGV, server_close_handler);
    signal(SIGTERM, server_close_handler);
    signal(SIGPIPE, spipe_handler);

    while (1) {
        // accetto la connessione da un client
        client_desc = accept(socket_desc, (struct sockaddr*) client, (socklen_t*) &sockaddr_len);
        if (client_desc == -1 && errno == EINTR) continue;
        else if (client_desc == -1) {
            printf("Errore nell'apertura della socket per la connessione entrante");
            continue;
        }

        // mando il messaggio di accettazione della connessione
        char_macro(buf, sizeof(buf), CONN_ACC);
        while ((ret = send(client_desc, buf, sizeof(buf), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send della connessione accettata del main()");
        }

        // creo un thread e gli affido il client
        pthread_t thread;
        if (pthread_create(&thread, NULL, conn_handler, &client_desc) != 0) {
            fprintf(stderr, "Impossibile creare un nuovo thread, errore %d\n", errno);
            exit(1);
        }
        pthread_detach(thread);

        client = calloc(1, sizeof(struct sockaddr_in));
    }
    server_close_handler();
}
