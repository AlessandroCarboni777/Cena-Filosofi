#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>

//CREAZIONE VARIABILI GLOBALI DI INPUT
int n_filosofi;
int flag_stallo;
int flag_soluzione_stallo;
int flag_starvation;

//CREAZIONE VARIABILI GLOBALI UTILI
char str[64];
sem_t sem; //semaforo
sem_t *forchetta[256];
struct timespec tempo;
int pipefd[2];
int counter_stallo = 0;

//DEFINIZIONI DI FUNZIONI.
void cancella_forchetta(int x) { //funzione chiamata in f_handler
  for (int i = 1; i <= x; i++) {
    sprintf(str, "%d", i);
    sem_close(forchetta[i]);
    sem_unlink(str);
  }
}

void f_handler(int iSignalCode) { //funzione per gestire CTRL+C
  printf("\n\n[CONSOLE MESSAGE]: handler ricevuto: (CTRL+C)\n");
  cancella_forchetta(n_filosofi);
  printf("\nIl ristorante sta chiudendo! Ci scusiamo per il disagio, arrivederci...\n\n\n");
  exit(0);
}

void mangia(sem_t * forchetta[], int i){
    while(1) {
        tempo.tv_sec = 0;
        tempo.tv_nsec = 5;
        if(flag_stallo == 1){
          //associazione filosofo-forchetta destra
          printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua DESTRA...", getpid());
          sem_wait(forchetta[i]);
          printf("\nIl filosofo %d HA PRESO la forchetta alla sua DESTRA!\n", getpid());
          fflush(stdout);

          printf("\n\nZIO CAGNONE");
          //associazione filosofo-forchetta sinistra
          printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua SINISTRA...", getpid());
            //gestione stallo
            read(pipefd[0], &counter_stallo, sizeof(int));
            counter_stallo++;
            printf("\n\nPROVA 1\n\n");
            write(pipefd[1], &counter_stallo, sizeof(int));
            if(counter_stallo == n_filosofi){
                printf("\n\n[CONSOLE MESSAGE]: stallo rilevato!\nQuesta sera i filosofi sono più affamati del solito...");
                cancella_forchetta(n_filosofi);
                exit(0);
            }

            sem_wait(forchetta[(i)%n_filosofi]);
            printf("\nIl filosofo %d HA PRESO la forchetta alla sua SINISTRA!\n", getpid());
            read(pipefd[0], &counter_stallo, sizeof(int));

            //gestione stallo
            counter_stallo--;
            write(pipefd[1], &counter_stallo, sizeof(int));
            printf("\n\nPROVA 2 %d\n\n", counter_stallo);

            printf("\nIl filosofo %d STA MANGIANDO...\n", getpid());
            nanosleep(&tempo, NULL);

            sem_post(forchetta[i]);
            printf("\nIl filosofo %d STA POSANDO la forchetta DESTRA...\n", getpid());
            sem_post(forchetta[i%n_filosofi]);
            printf("\nIl filosofo %d STA POSANDO la forchetta SINISTRA...\n", getpid());
        }

        //inizio soluzione stallo
        int destra, sinistra;

        if(flag_soluzione_stallo <= 0){
            printf("[CONSOLE MESSAGE]: flag della soluzione allo stallo nulla! Il ristorante chiude i battenti...");
            exit(0);
        } else { //cambio destra e sinistra
            int destra = i;
            int sinistra = i % n_filosofi;

            if(i == n_filosofi - 1){
                destra = i%n_filosofi;
                sinistra = i;
            }
        }

            printf("il filosofo con pid %d ATTENDO la forchetta%d\n", getpid(), destra);
            sem_wait(forchetta[destra]);
            printf("il filosofo con pid %d HA PRESO la forchetta\n", getpid());

            printf("il Filosofo con pid %d ATTENDO la forchetta%d\n", getpid(), sinistra);
            sem_wait(forchetta[sinistra]);
            printf("il filosofo con pid %d HA PRESO la forchetta\n", getpid());

            printf("sta mangiando filosofo %d\n", getpid());
            nanosleep(&tempo, NULL);

            sem_post(forchetta[destra]);
            printf("il filosofo con pid %d sta LASCIANDO la forchetta%d\n", getpid(), destra);
            sem_post(forchetta[sinistra]);
            printf("il filosofo con pid %d sta LASCIANDO la forchetta%d\n", getpid(), sinistra);
        }
    }

//INIZIO MAIN.
int main(int argc, char *argv[]) {

//ASSEGNAZIONE VALORI DEI FLAG E GESTIONE DEGLI ERRORI DI INPUT.
  if(argc < 2) { //Nessun argomento passato.
    printf("\n\n[ERROR]: numero di filosofi non specificato!");
    exit(-1);
  } else if(argc<=2) { //Un solo argomento passato.
    printf("\n\n[WARNING]: è stato inserito solo il numero di filosofi!");
    n_filosofi = atoi(argv[1]);
    if(n_filosofi<=0) {
      printf("\n\n[ERROR]: il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d", n_filosofi);
  } else if(argc<=3) { //Solo due argomenti passati.
    printf("\n\n[WARNING]: sono stati inseriti solo il numero di partecipanti ed il flag dello stallo!");
    n_filosofi = atoi(argv[1]);
    flag_stallo = atoi(argv[2]);
    if(n_filosofi<=0) {
      printf("\n\n[ERROR]: il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo %d: ", n_filosofi, flag_stallo);
  } else if(argc<=4) { //Solo tre argomenti passati.
    printf("\n\n[WARNING]: sono stati inseriti solo il numero di partecipanti, il flag dello stallo ed il flag per la soluzione allo stallo!");
    n_filosofi = atoi(argv[1]);
    flag_stallo = atoi(argv[2]);
    flag_soluzione_stallo = atoi(argv[3]);
    if(n_filosofi<=0) {
      printf("\n\n[ERROR]: il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d\nFlag della soluzione allo stallo: %d", n_filosofi, flag_stallo, flag_soluzione_stallo);
  } else if(argc<=5) { //Tutti gli argomenti passati correttamente.
    n_filosofi = atoi(argv[1]);
    flag_stallo = atoi(argv[2]);
    flag_soluzione_stallo = atoi(argv[3]);
    flag_starvation = atoi(argv[4]);
    if(n_filosofi<=0) {
      printf("\n\n[ERROR]: il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d\nFlag della soluzione allo stallo: %d\nFlag della starvation: %d", n_filosofi, flag_stallo, flag_soluzione_stallo, flag_starvation);
  } else if(argc>5) {
    printf("\n\n[WARNING]: troppi elementi specificati, solo i primi 4 verranno considerati!");
    n_filosofi = atoi(argv[1]);
    flag_stallo = atoi(argv[2]);
    flag_soluzione_stallo = atoi(argv[3]);
    flag_starvation = atoi(argv[4]);
    if(n_filosofi<=0) {
      printf("\n\n[ERROR]: il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d\nFlag della soluzione allo stallo: %d\nFlag della starvation: %d", n_filosofi, flag_stallo, flag_soluzione_stallo, flag_starvation);
  }

//CREAZIONE DELLA STRUCT SIGACTION.
  struct sigaction sa; //struttura sigaction
  memset(&sa, '\0', sizeof(struct sigaction));
  sa.sa_handler = f_handler; //chiamata funzione per interruzione con CTRL+C
  sigaction(SIGINT, &sa, NULL);

  printf("\n\n\n Il Ristorante è aperto! Aspettando i filosofi...\n\n");
  sleep(5);

//STAMPE A VIDEO NEL CASO DI FLAG NULLI O NEGATIVI.
  if (flag_stallo==0) {
    printf("\n\n[WARNING]: flag dello stallo nulla, verrà disattivata!\n");
  } else if (flag_stallo < 0) {
    printf("\n\n[WARNING]: flag dello stallo negativa, verrà disattivata!\n");
  }
  if (flag_soluzione_stallo==0) {
    printf("\n\n[WARNING]: flag della soluzione allo stallo nulla, verrà disattivata!\n");
  } else if (flag_soluzione_stallo < 0) {
    printf("\n\n[WARNING]: flag della soluzione allo stallo negativa, verrà disattivata!\n");
  }
  if (flag_starvation==0) {
    printf("\n\n[WARNING]: flag della starvation nulla, verrà disattivata!\n");
  } else if (flag_starvation < 0) {
    printf("\n\n[WARNING]: flag della starvazion negativa, verrà disattivata!\n");
  }

//CREAZIONE DELLA PIPE PER LO STALLO
    if(pipe(pipefd) == -1) {
      perror("\n\n[FATAL ERROR]: i filosofi non riescono a mettersi d'accordo!\n");
      exit(EXIT_FAILURE);
    }

//CREAZIONE DEI SEMAFORI (FORCHETTE).
  for(int i = 1; i <= n_filosofi; i++){ //ciclo for per le forchette
    sprintf(str, "%d", i);
    if((forchetta[i] = sem_open(str, O_CREAT, 1)) == SEM_FAILED){
      perror("\n\n[CONSOLE MESSAGE]: Manca una forchetta a tavola!\n");
      exit(EXIT_FAILURE);
    }
  }

//CREAZIONE DEI PROCESSI CON FORK.
  pid_t filosofo[n_filosofi]; //array di filosofi

  for(int i = 1; i <= n_filosofi; i++){
    filosofo[i] = fork();
    if(filosofo[i] == -1) {
  		 perror("\n\n[FATAL ERROR]: uno dei filosofi non è potuto venire, la cena è rimandata!\n");
  		 exit(EXIT_FAILURE);
    } else if(filosofo[i] == 0) { //Entro in uno dei child.
        printf("\nSi è unito un nuovo filosofo a tavola. Numero del filosofo: %d\n", getpid());
        mangia(forchetta, i);
      return 0; //Esco dal child quando ha finito.
    }
  }

  for(int i = 1; i <= n_filosofi; i++) {
    waitpid(filosofo[i], NULL, 0);
  }

  printf("Il ristorante sta chiudendo, arrivederci!");

}
