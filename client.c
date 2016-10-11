#include "utility.h"

// variabili globali
int socket_desc;
char mitt[USR_SIZE];

/**********************************************************************************
*   register_user(): funzione che gestisce la registrazione di un nuovo utente	  *
*		             alla bacheca.                                                *
**********************************************************************************/

int register_user() {
    char cmd[2], resp[3];
    int ret, msg_len;

    // invio del numero del comando al server
    cmd[0] = '5';
    while ((ret = send(socket_desc, cmd, sizeof(cmd), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del comando in register_user()");
    }

    // ricezione dell'ok da parte del server
    while ((msg_len = recv(socket_desc, resp, sizeof(resp), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok iniziale in register_user)");
    }

    // controllo di aver ricevuto OK/KO
    if (strcmp(resp, KO) == 0) ERROR_HELPER(-1, "Impossibile effettuare la registrazione "
                                                "in register_user()");
    else {
        char fin[MAX];
        re_reg:
        restr(fin, sizeof(fin));

        // input delle credenziali
        printf("Inserire i nuovi nome utente e password (separati fra loro da uno spazio): ");
        fgets(fin, sizeof(fin), stdin);

        // mando le credenziali al server
        while ((ret = send(socket_desc, fin, sizeof(fin), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(ret, "Errore nella send delle credenziali del metodo register_user()");
        }

        // mi salvo il mittente
        STR token = strtok(fin, " ");
        strncpy(mitt, token, sizeof(mitt));

        printf("In attesa che il server processi la richiesta...");

        // ricezione dell'esito della registrazione da parte del server
        restr(resp, sizeof(resp));
        while ((msg_len = recv(socket_desc, resp, sizeof(resp), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(msg_len, "Errore nella receive dell'ok finale in register_user()");
        }

        if (strcmp(resp, KK) == 0) {
            printf("Il nome utente inserito esiste già, sceglierne un altro.\n");
            goto re_reg;
        } else if (strcmp(resp, OK) == 0) {
            printf("\nRegistrazione avvenuta correttamente.\n");
            return 0;
        } else ERROR_HELPER(-1, "Risposta finale dal server negativa in register_user()");
    }
}

/**********************************************************************************
*   conn_close(): funzione che gestisce la chiusura della connessione quando 	  *
*                 l'utente decide di terminare la sessione.                       *
**********************************************************************************/

void conn_close() {
    int tmp, ret, msg_len, r;
    char cmd[2], resp[3];

    r = sizeof(resp);

    // invio del numero del comando al server
    cmd[0] = '4';
    while ((ret = send(socket_desc, cmd, sizeof(cmd), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del comando in conn_close()");
    }

    // ricezione dell'esito della chiusura di connessione da parte del server
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok iniziale in conn_close()");
    }
    if (strcmp(resp, OK) != 0) ERROR_HELPER(-1, "Errore durante la chiusura della connessione in "
                "conn_close()");

    tmp = close(socket_desc);
    ERROR_HELPER(tmp, "Errore nella chiusura della socket");
    printf("_________________Connessione chiusa_________________\n");

    exit(EXIT_SUCCESS);
}

/**********************************************************************************
*   client_close(): funzione che gestisce la chiusura della connessione quando 	  *
*                 il client riceve un segnale.                                    *
**********************************************************************************/

void client_close() {
    int ret;

    ret = close(socket_desc);
    ERROR_HELPER(ret, "Errore nella chiusura della socket");
    printf("\n_________________Connessione chiusa_________________\n");

    exit(EXIT_SUCCESS);
}

/**********************************************************************************
*   login(): funzione che gestisce l'autenticazione dell'utente.                  *
**********************************************************************************/

int login() {
    char cmd[2], resp[3], answer[MAX], c;
    int j = 0, ret, msg_len, r;

    r = sizeof(resp);

    // invio del numero del comando al server
    cmd[0] = '0';
    while ((ret = send(socket_desc, cmd, sizeof(cmd), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del comando in login()");
    }

    // ricezione dell'OK da parte del server
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok iniziale in login()");
    }
    if (strcmp(resp, OK) != 0) ERROR_HELPER(-1, "Impossibile effettuare il login()");
    restr(answer, sizeof(answer));

    printf("\n_________________LOGIN_________________\n");
    printf("\nInserire username e password (separati fra loro da uno spazio): ");
    while ((c = fgetc(stdin)) != ESCAPE) {
        answer[j] = c;
        j++;
    }

    // invio le credenziali al server
    while ((ret = send(socket_desc, answer, sizeof(answer), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send delle credenziali in login()");
    }

    // mi salvo il mittente
    STR token = strtok(answer, " ");
    strncpy(mitt, token, sizeof(mitt));

    printf("In attesa che il server processi la richiesta...\n");

    // ricezione dell'esito del login da parte del server
    restr(resp, sizeof(resp));
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok finale in login()");
    }
    if (strcmp(resp, OK) == 0) return 0;
    else if (strcmp(resp, KK) == 0) return 1;
    else return -1;
}

/**********************************************************************************
*   read_board(): funzione che permette all'utente di leggere tutti i messaggi 	  *
*                 presenti nella bacheca, aggiunti da qualunque utente.           *
**********************************************************************************/

int read_board() {
    int i, ret, msg_len, r;
    long nlines;
    char cmd[2], resp[3], line[MAX], lines[8];

    r = sizeof(resp);

    // invio del numero del comando al server
    cmd[0] = '1';
    while ((ret = send(socket_desc, cmd, sizeof(cmd), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del comando in read_board()");
    }

    // ricezione dell'ok da parte del server
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok iniziale in read_board()");
    }
    if (strcmp(resp, OK) != 0) ERROR_HELPER(-1, "Impossibile leggere la bacheca");

    printf("In attesa che gli altri utenti finiscano di aggiornare la bacheca...\n");

    // ricevo il numero di righe da stampare
    while ((msg_len = recv(socket_desc, lines, sizeof(lines), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive del numero di righe della bacheca in"
                                " read_board()");
    }
    nlines = strtol(lines, NULL, 10);

    // stampo le nlines righe che ricevo
    printf("____________________ BACHECA ____________________\n\n");
    for (i = 0; i < nlines; ++i) {
        fflush(stdout); // svuota il buffer associato allo stdout prima di stampare le righe
        if ((msg_len = recv(socket_desc, line, sizeof(line), 0)) < 0) {
            if (errno == EINTR) continue;
            ERROR_HELPER(msg_len, "Errore nella receive delle righe della bacheca in read_board()");
        }
        printf("%s", line);
    }
    printf("_________________________________________________\n");

    return 0;
}

/**********************************************************************************
*   add_msg(): funzione che permette all'utente di aggiungere un messaggio alla   *
*              bacheca, che potranno visualizzare tutti gli utenti.               *
**********************************************************************************/

int add_msg() {
    int j, ret, msg_len, r;
    char c, cmd[2], resp[3], obj[OBJ_SIZE], txt[TXT_SIZE];

    r = sizeof(resp);

    // invio del numero del comando al server
    cmd[0] = '2';
    while ((ret = send(socket_desc, cmd, sizeof(cmd), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del comando in add_msg()");
    }

    // ricezione dell'ok da parte del server
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok iniziale in add_msg()");
    }
    if (strcmp(resp, OK) != 0) ERROR_HELPER(-1, "Impossibile aggiungere un nuovo messaggio in "
                                                "add_msg()");
    // mando il mittente
    re_mit:
    while ((ret = send(socket_desc, mitt, sizeof(mitt), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del mittente in add_msg()");
    }

    printf("In attesa che gli altri utenti finiscano di aggiornare la bacheca...\n");

    // ricezione della risposta da parte del server
    restr(resp, sizeof(resp));
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok mittente in add_msg()");
    }
    if (strcmp(resp, OK) == 0) printf("Mittente ricevuto correttamente da parte del server.\n");
    else {
        printf("ERRORE: mittente non ricevuto correttamente, tentativo in corso...\n");
        goto re_mit;
    }

    printf("Premere ENTER per inviare i campi al server.\n");

    // acquisizione e invio dell'oggetto del messaggio
    re_obj:
    j = 0;
    restr(obj, sizeof(obj));
    printf("Inserire oggetto (max 100 caratteri): ");
    while ((c = fgetc(stdin)) != ESCAPE) {
        obj[j] = c;
        j++;
    }
    while ((ret = send(socket_desc, obj, sizeof(obj), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'oggetto in add_msg()");
    }

    printf("In attesa che gli altri utenti finiscano di aggiornare la bacheca...\n");

    // ricezione della risposta da parte del server
    restr(resp, sizeof(resp));
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'oggetto da parte del server in add_msg()");
    }
    if (strcmp(resp, OK) == 0) printf("\nOggetto ricevuto correttamente da parte del server.\n");
    else {
        if (strcmp(resp, KK) == 0) printf("\nNel file è già presente un messaggio con l'oggetto "
                                            "immesso. Prego inserirne uno nuovo.\n");
        else printf("\nERRORE: oggetto non ricevuto correttamente, tentativo in corso...\n");
        goto re_obj;
    }

    // acquisizione e invio del testo del messaggio
    j = 0;
    restr(txt, sizeof(txt));
    printf("Inserire testo (max 4000 caratteri): ");
    while ((c = fgetc(stdin)) != ESCAPE) {
        txt[j] = c;
        j++;
    }
    while ((ret = send(socket_desc, txt, sizeof(txt), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del testo in add_msg()");
    }

    printf("In attesa che gli altri utenti finiscano di aggiornare la bacheca...\n");

    // ricezione della risposta da parte del server
    restr(resp, sizeof(resp));
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok finale in add_msg()");
    }
    if (strcmp(resp, OK) == 0) printf("\nTesto ricevuto correttamente da parte del server.\n");
    else printf("\nERRORE: testo non ricevuto correttamente. Si prega di riprovare.\n");
    return 0;
}

/**********************************************************************************
*   del_msg(): funzione che permette all'utente di cancellare un messaggio dalla  *
*              bacheca SOLO se aggiunto da lui (non si possono cancellare i 	  *
*              messaggi inviati da altri utenti).                                 *
**********************************************************************************/

int del_msg() {
    int j = 0, ret, msg_len, r;
    char c, cmd[2], resp[3], obj[OBJ_SIZE];

    r = sizeof(resp);

    // invio del numero del comando al server
    cmd[0] = '3';
    while ((ret = send(socket_desc, cmd, sizeof(cmd), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del comando in del_msg()");
    }

    // ricezione dell'ok da parte del server
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella receive dell'ok iniziale in del_msg()");
    }
    if (strcmp(resp, OK) != 0) ERROR_HELPER(-1, "Impossibile cancellare messaggi");

    // mando il mittente
    remit:
    while ((ret = send(socket_desc, mitt, sizeof(mitt), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send del mittente in del_msg()");
    }

    printf("In attesa che gli altri utenti finiscano di aggiornare la bacheca...\n");

    // ricezione della risposta da parte del server
    restr(resp, sizeof(resp));
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella ricezione dell'ok mittente in del_msg()");
    }
    if (strcmp(resp, OK) == 0) printf("Mittente ricevuto correttamente da parte del server.\n");
    else {
        printf("ERRORE: mittente non ricevuto correttamente, tentativo in corso...\n");
        goto remit;
    }

    // acquisizione dell'oggetto del messaggio da cancellare
    restr(obj, sizeof(obj));
    printf("Inserire l'oggetto del messaggio da cancellare: ");
    while ((c = fgetc(stdin)) != ESCAPE) {
        obj[j] = c;
        j++;
    }

    // invio dell'oggetto
    while ((ret = send(socket_desc, obj, sizeof(obj), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(ret, "Errore nella send dell'oggetto in del_msg()");
    }

    printf("In attesa che gli altri utenti finiscano di aggiornare la bacheca...\n");

    // ricezione della risposta da parte del server
    restr(resp, sizeof(resp));
    while ((msg_len = recv(socket_desc, resp, r, 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella ricezione dell'ok finale in del_msg()");
    }
    if (strcmp(resp, OK) == 0) printf("Messaggio cancellato correttamente da parte del server.\n");
    else if (strcmp(resp, KK) == 0) printf("L'utente non possiede l'autorizzazione per cancellare "
                                            "il messaggio.\n");
    else if (strcmp(resp, NE) == 0) printf("Non esiste alcun messaggio con l'oggetto inserito.\n");
    else ERROR_HELPER(-1, "Errore nella ricezione della risposta finale del server.");

    return 0;
}

/**********************************************************************************
*   funzione main(int argc, char** argv): provvede alla connessione con il server:*
                    prende come paramentro l'indirizzo IP del server in forma     *
                    "dotted"; se non riceve alcun parametro, si connette alla     *
                    macchina corrente (127.0.0.1).                                *
**********************************************************************************/

int main(int argc, char** argv) {
    int auth = 0, j, count = 0, tmp, ret, msg_len;
    char answer[2], cmmd[2], buf[BUF_SIZE];

    // apertura socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Errore nell'apertura della socket");

    // struct dell'indirizzo di destinazione
    struct sockaddr_in server = {0};
    if (argc == 1) server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    else server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    // connessione con il server
    ret = connect(socket_desc, (struct sockaddr*) &server, sizeof(server));
    ERROR_HELPER(ret, "Errore nella connessione con il server");

    // attende da risposta del server (se può o non può connettersi)
    while ((msg_len = recv(socket_desc, buf, sizeof(buf), 0)) < 0) {
        if (errno == EINTR) continue;
        ERROR_HELPER(msg_len, "Errore nella ricezione della risposta del server");
    } // il server deve mandare una risposta alla ricezione della connessione
    if (strcmp(buf, CONN_REF) == 0) {
        printf("Connessione rifiutata dal server. Terminazione della sessione...\n");
        conn_close();
    } else printf("_________________Server Connesso_________________\n");

    // assegna la ruotine ai vari segnali
    signal(SIGHUP, client_close);
    signal(SIGINT, client_close);
    signal(SIGQUIT, client_close);
    signal(SIGILL, client_close);
    signal(SIGSEGV, client_close);
    signal(SIGTERM, client_close);
    signal(SIGPIPE, SIG_IGN);

    // autenticazione (3 tentativi)/registrazione
    do {
        j = 0;
        char c;
        restr(answer, sizeof(answer));
        printf("Nuovo utente (Y/N)? ");
        while ((c = fgetc(stdin)) != ESCAPE) {
            answer[j] = c;
            j++;
        }
        if (strcmp(answer, "Y") == 0 || strcmp(answer, "y") == 0) {
            tmp = register_user();
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nella registrazione");
            auth = 1;
        } else if (strcmp(answer, "N") == 0 || strcmp(answer, "n") == 0) {
            log:
            tmp = login();
            if (tmp < 0) {
                if (count == 2) { // superamento dei 3 tentativi
                    printf("\nSono stati superati i 3 tentativi. \nImpossibile autenticare "
                            "l'utente. \nTermine della connessione...\n");
                    conn_close();
                    return -1;
                }
                count++;
                printf("\nAutenticazione non avvenuta, riprova. "
                        "Rimangono %d tentativi...\n", 3 - count);
                goto log;
            } else if (tmp == 1) { // non esiste un utente con lo username inserito
                printf("\nNon esiste un utente con questo username. Procedere con la registrazione "
                        "(Y/N)? ");
                char d, ans[2];
                int k = 0;
                restr(ans, sizeof(ans));
                while ((d = fgetc(stdin)) != ESCAPE) {
                    ans[k] = d;
                    k++;
                }
                if (strcmp(ans, "Y") == 0 || strcmp(ans, "y") == 0) {
                    tmp = register_user();
                    if (tmp < 0) ERROR_HELPER(tmp, "Errore nella registrazione");
                    auth = 1;
                } else if (strcmp(ans, "N") == 0 || strcmp(ans, "n") == 0) {
                    goto log;
                } else printf("\nCarattere inserito non valido.\n");
            } else { // autenticato
                printf("\nAutenticazione avvenuta correttamente.\n");
                auth = 1;
            }
        } else printf("\nCarattere inserito non valido.\n");
    } while (auth == 0);

    // loop dei comandi
    char c;
    while (1) {
        printf("\n"
               "*************** LISTA DEI COMANDI ***************\n"
               "*   1 - Leggere i messaggi presenti in bacheca	*\n"
               "*   2 - Inserire un nuovo messaggio in bacheca	*\n"
               "*   3 - Cancellare un messaggio dalla bacheca	*\n"
               "*   4 - Terminare il programma                  *\n"
               "*************************************************\n\n");
        try_again:
        j = 0;
        restr(cmmd, sizeof(cmmd));
        printf("Inserire il numero corrispondente ad un comando: ");
        while ((c = fgetc(stdin)) != ESCAPE) {
            cmmd[j] = c;
            j++;
        }
        printf("\n");
        if (strcmp(cmmd, "1") == 0) {
            tmp = read_board();
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nella lettura del messaggio");
        } else if (strcmp(cmmd, "2") == 0) {
            tmp = add_msg();
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nell'aggiunta del messaggio");
        } else if (strcmp(cmmd, "3") == 0) {
            tmp = del_msg();
            if (tmp < 0) ERROR_HELPER(tmp, "Errore nella cancellazione del messaggio");
        } else if (strcmp(cmmd, "4") == 0) {
            conn_close();
        } else {
            printf("Comando non valido. Riprovare. \n");
            goto try_again;
        }
    }
    client_close();
}
