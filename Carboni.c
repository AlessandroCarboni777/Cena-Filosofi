//PROGETTO CARBONI ALESSANDRO matricola: 342254


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

//defines
#define RED "\x1b[31m"
#define BLU "\x1b[36m"
#define MAGENTA "\x1b[35m"
#define RESET "\x1b[0m"


//CREAZIONE VARIABILI GLOBALI DI INPUT
int n_filosofi = 0;
int flag_stallo = 0;
int flag_soluzione_stallo = 0;
int flag_starvation = 0;

//CREAZIONE VARIABILI GLOBALI UTILI
char str[64];
sem_t sem; //semaforo
sem_t *forchetta[256];
sem_t *forchetta_mod[256];
int destra, sinistra = 0;
struct timespec tempo;
struct timespec tempo1;
int pipefd[2];
int counter_stallo = 1;

//DEFINIZIONI DI FUNZIONI.
void cancella_forchetta(int x) { //funzione chiamata in f_handler per chiudere tutti i semafori e file descriptor
  for (int i = 1; i <= x; i++) {
    sprintf(str, "%d", i);
    sem_close(forchetta[i]); //chiude il semafori i-esimo
    sem_unlink(str); //elimina il linkage del semaforo di nome "str"
  }
  close(pipefd[0]); //chiude la pipe in lettura
  close(pipefd[1]); //chiude la pipe in scrittura
}

void f_handler(int iSignalcode) { //funzione per gestire CTRL+C
  printf(BLU"\n\n[CONSOLE MESSAGE]"RESET": Arrivederci filosofo numero %d\n", getpid());
  cancella_forchetta(n_filosofi);
  return;
}

void mangia_solo_filosofi(sem_t *forchetta[], int i) { //funzione che viene eseguita solo nel caso in cui viene inserito solo il numero di filosofi valido
  tempo.tv_sec = 1; //definizioni delle componenti della struct tempo (secondi)
  tempo.tv_nsec = 5; //(nanosecondi)

  while(1) { //ciclo infinito per la continua esecuzione

    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua DESTRA...", getpid());
    sem_wait(forchetta[i]); //il semaforo (forchetta destra) sta aspettando di essere "preso"
    printf("\nIl filosofo %d HA PRESO la forchetta alla sua DESTRA!\n", getpid());
    fflush(stdout); //cancella il buffer di output

    sleep(1); //aiuta l'esecuzione del programma

    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua SINISTRA...", getpid());
    sem_wait(forchetta[i%n_filosofi]); //il semaforo (forchetta sinistra) sta aspettando di essere "preso"
    printf("\nIl filosofo %d HA PRESO la forchetta alla sua SINISTRA!\n", getpid());

    printf("sta mangiando filosofo %d\n", getpid());
    nanosleep(&tempo, NULL); //sleep piccola dopo aver mangiato

    sem_post(forchetta[i]); //libera il semaforo "di destra"
    printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
    sem_post(forchetta[i%n_filosofi]); //"libera il semaforo di sinistra"
    printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
  }
}

void mangia_con_stallo(sem_t *forchetta[], int i) { //funzione eseguita solo se ho un numero di filosofi valido e il flag di stallo positiva
  tempo.tv_sec = 1; //definizioni delle componenti della struct tempo (secondi)
  tempo.tv_nsec = 5;

  while(1) { //ciclo infinito

    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua DESTRA...", getpid());
    sem_wait(forchetta[i]); //aspetta il semaforo "di destra"
    printf("\nIl filosofo %d HA PRESO la forchetta alla sua DESTRA!\n", getpid());
    fflush(stdout); //cancella il buffer di output

    sleep(1); //aiuta lo stallo

    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua SINISTRA...", getpid());
    read(pipefd[0], &counter_stallo, sizeof(int)); //legge nella pipe
    counter_stallo++; //aumenta il counter che conta lo stallo
    printf(BLU"\n\n[CONSOLE MESSAGE]"RESET": counter stallo aumentata!\n\n");
    write(pipefd[1], &counter_stallo, sizeof(int)); //scrive il valore aggiornato sulla pipe (visibile da tutti i child)

    if(counter_stallo == n_filosofi) { //se la variabile counter stallo è uguale al numero di filosofi, insorge stallo poichè ognuno ha una forchetta in mano
        printf(RED"\n\n[FATAL ERROR]" RESET": stallo rilevato!\nQuesta sera i filosofi sono più affamati del solito...\n\n");
        kill(0, SIGINT); //uccide tutti i child
        exit(1);
    }

    sem_wait(forchetta[i%n_filosofi]); //aspetta il semaforo "di sinistra"
    printf("\nIl filosofo %d HA PRESO la forchetta alla sua SINISTRA!\n", getpid());
    read(pipefd[0], &counter_stallo, sizeof(int)); //legge la pipe

    //gestione stallo
    counter_stallo--; //dimiuisce il counter dello stallo
    write(pipefd[1], &counter_stallo, sizeof(int)); //scrive la variabile counter aggiornata nella pipe
    printf(BLU"[CONSOLE MESSAGE]"RESET": counter stallo decrementata!\n\n");

    printf("\nIl filosofo %d STA MANGIANDO...\n", getpid());
    nanosleep(&tempo, NULL); //piccola sleep dopo aver mangiato

    sem_post(forchetta[i]); //libera il semaforo "di destra"
    printf("il filosofo con pid %d HA LASCIATO la forchetta DESTRA!\n", getpid());
    sem_post(forchetta[i%n_filosofi]); //libera il semaforo "di sinsitra"
    printf("il filosofo con pid %d HA LASCIATO la forchetta SINISTRA!\n", getpid());

  }
}

void mangia_senza_stallo(sem_t *forchetta[], int i) { //funzione usata solo nel caso in cui ho la flag della soluzione allo stallo positive e flag della starvation nulla
  tempo.tv_sec = 1; //definizioni delle componenti della struct tempo (secondi)
  tempo.tv_nsec = 5; //(nanosecondi)

  while(1) { //ciclo infinito

    //assegno manualmente le forchette destra e sinistra ai singoli filosofi, portando particolare attenzione ai filosofi "critici" (primo e ultimo)
      if(i == 1) {
        sinistra = n_filosofi;
        destra = i;
      } else if(i == n_filosofi) { //l'ultimo filosofo che si siede al tavolo avrà la forchetta destra e sinistra invertite (come indicato nel testo)
          destra = i-1;
          sinistra = 1;
      } else {
          destra = i;
          sinistra= i-1;
      }

      printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua DESTRA...", getpid());
      sem_wait(forchetta[destra]); //attesa del semaforo "di destra"
      printf("\nIl filosofo %d HA PRESO la forchetta alla sua DESTRA!\n", getpid());
      fflush(stdout); //cancella il buffer di output

      printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua SINISTRA...", getpid());
      sem_wait(forchetta[sinistra]); //aspetta il semaforo "di sinsitra"
      printf("\nIl filosofo %d HA PRESO la forchetta alla sua SINISTRA!\n", getpid());

      printf("\nIl filosofo %d STA MANGIANDO...\n", getpid());
      nanosleep(&tempo, NULL); //nano sleep dopo aver mangiato

      sem_post(forchetta[destra]); //libera il semaforo "di destra"
      printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
      sem_post(forchetta[sinistra]); //libera il semaforo "di sinistra"
      printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
  }
}

void mangia_con_starvation(sem_t *forchetta[], int i) { //funzione chiamata in cui ho le flag della soluzione allo stallo e della starvation positive
  int timer1 = 0; //imposto due timer di indice intero, che serviranno per verificare se ho raggiunto il tempo limite (8 secodni, come inidcato dal testo)
  int timer2 = 0;

//ciclo identico a quello della funzione "mangia_senza_stallo()"
  if(i == 1) {
    sinistra = n_filosofi;
    destra = i;
  } else if(i == n_filosofi) {
      destra = 1;
      sinistra = i-1;
  } else {
      destra = i;
      sinistra= i-1;
  }

  struct timespec tempo_starvation; //struct per impostare il tempo della starvation
  tempo.tv_nsec = 5; //impostazione nanosecondi di d-lay

  while(1) { //ciclo infinito
    clock_gettime(CLOCK_REALTIME, &tempo_starvation); //associazione a tempo_starvation del "tempo universale" in continuo aumento dal 1° gennaio 1970
    tempo_starvation.tv_sec +=8; //aumento del counter tempo_starvation di 8 secondi (testo)

    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua DESTRA...", getpid());
    timer1 = sem_timedwait(forchetta[destra], &tempo_starvation);  //attesa del semaforo "destro" che attende il tempo specificato da tempo_starvation
    if(timer1 == -1) { //se il timer è negativo significa che viene decrementato un valore maggiore al tempo della variabile tempo_starvation, quindi si insorge nel problema di starvation (per la forchetta destra)
      if(errno == ETIMEDOUT) { //tipo id errore che "corrisponde" alla starvation
        printf(RED"\n\n[FATAL ERROR]" RESET": rilevata starvation!\n\n");
        kill(0, SIGINT); //chiama la kill con il segnale SIGINT che cancella tutte le forchette e chiude tutti i processi figli
      }
    }

    printf("\nIl filosofo %d HA PRESO la forchetta alla sua DESTRA!\n", getpid());
    fflush(stdout); //cancella il buffer di lettura

    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua SINISTRA...", getpid());
    timer2 = sem_timedwait(forchetta[sinistra], &tempo_starvation); //attesa del semaforo "sinistro" che attende il tempo specificato da tempo_starvation
    if(timer2 == -1) { //se il timer è negativo significa che viene decrementato un valore maggiore al tempo della variabile tempo_starvation, quindi si insorge nel problema di starvation (per la forchetta sinsitra)
      if(errno == ETIMEDOUT) { //tipo id errore che "corrisponde" alla starvation
        printf(RED"\n\n[FATAL ERROR]"RESET": rilevata starvation!\n\n");
        kill(0, SIGINT); //chiama la kill con il segnale SIGINT che cancella tutte le forchette e chiude tutti i processi figli
      }
    }

    printf("\nIl filosofo %d HA PRESO la forchetta alla sua SINISTRA!\n", getpid());

    printf("\nIl filosofo %d STA MANGIANDO...\n", getpid());
    nanosleep(&tempo, NULL); //nanosleep dopo aver mangiato

    sem_post(forchetta[destra]); //libera il semaforo "di destra"
    printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
    sem_post(forchetta[sinistra]); //libera il semaforo "di sinsitra"
    printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
  }
}

void mangia_stallo_e_starvation(sem_t *forchetta[], int i) {

  //creo una array di semafori uguali a forchetta[]
  for(int j = 1; j <= n_filosofi; j++) {
    forchetta_mod[j] = forchetta[j];
  }

  int timer1 = 0; //imposto due timer di indice intero, che serviranno per verificare se ho raggiunto il tempo limite (8 secodni, come inidcato dal testo)
  int timer2 = 0;

  struct timespec tempo_starvation;
  tempo.tv_sec = 1; //definizioni delle componenti della struct tempo (secondi)
  tempo.tv_nsec = 5; //(nanosecondi)

  if(i == 1) {
    sinistra = n_filosofi;
    destra = i;
  } else if(i == n_filosofi) {
      destra = 1;
      sinistra = i-1;
  } else {
      destra = i;
      sinistra= i-1;
  }

  while(1) {

    clock_gettime(CLOCK_REALTIME, &tempo_starvation); //associazione a tempo_starvation del "tempo universale" in continuo aumento dal 1° gennaio 1970
    tempo_starvation.tv_sec +=8; //aumento del counter tempo_starvation di 8 secondi (testo)

    //verifico se si presenta starvation
    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua DESTRA...", getpid());
    sem_wait(forchetta[i]); //aspetta il semaforo "di destra"

    //controllo starvation
    timer1 = sem_timedwait(forchetta_mod[destra], &tempo_starvation);  //attesa del semaforo "destro" che attende il tempo specificato da tempo_starvation
    if(timer1 == -1) { //se il timer è negativo significa che viene decrementato un valore maggiore al tempo della variabile tempo_starvation, quindi si insorge nel problema di starvation (per la forchetta destra)
      if(errno == ETIMEDOUT) { //tipo id errore che "corrisponde" alla starvation
        printf(RED"\n\n[FATAL ERROR]" RESET": rilevata starvation!\n\n");
        kill(0, SIGINT); //chiama la kill con il segnale SIGINT che cancella tutte le forchette e chiude tutti i processi figli
      }
    }

    printf("\nIl filosofo %d HA PRESO la forchetta alla sua DESTRA!\n", getpid());
    fflush(stdout); //cancella il buffer di lettura

    sleep(1);

    //verifica se si presenta stallo
    read(pipefd[0], &counter_stallo, sizeof(int)); //legge nella pipe
    counter_stallo++; //aumenta il counter che conta lo stallo
    printf(BLU"\n\n[CONSOLE MESSAGE]"RESET": counter stallo aumentata!\n\n");
    write(pipefd[1], &counter_stallo, sizeof(int)); //scrive il valore aggiornato sulla pipe (visibile da tutti i child)

    if(counter_stallo == n_filosofi) { //se la variabile counter stallo è uguale al numero di filosofi, insorge stallo poichè ognuno ha una forchetta in mano
        printf(RED"\n\n[FATAL ERROR]" RESET": stallo rilevato!\nQuesta sera i filosofi sono più affamati del solito...\n\n");
        kill(0, SIGINT); //uccide tutti i child
        exit(1);
    }

    //verifica se si presenta starvation
    printf("\nIl filosofo %d STA ASPETTANDO per prendere la forchetta alla sua SINISTRA...", getpid());
    timer2 = sem_timedwait(forchetta_mod[sinistra], &tempo_starvation); //attesa del semaforo "sinistro" che attende il tempo specificato da tempo_starvation
    if(timer2 == -1) { //se il timer è negativo significa che viene decrementato un valore maggiore al tempo della variabile tempo_starvation, quindi si insorge nel problema di starvation (per la forchetta sinsitra)
      if(errno == ETIMEDOUT) { //tipo id errore che "corrisponde" alla starvation
        printf(RED"\n\n[FATAL ERROR]"RESET": rilevata starvation!\n\n");
        kill(0, SIGINT); //chiama la kill con il segnale SIGINT che cancella tutte le forchette e chiude tutti i processi figli
      }
    }

    sem_wait(forchetta[i%n_filosofi]); //aspetta il semaforo "di sinistra"
    printf("\nIl filosofo %d HA PRESO la forchetta alla sua SINISTRA!\n", getpid());
    read(pipefd[0], &counter_stallo, sizeof(int)); //legge la pipe

    counter_stallo--; //dimiuisce il counter dello stallo
    write(pipefd[1], &counter_stallo, sizeof(int)); //scrive la variabile counter aggiornata nella pipe
    printf(BLU"[CONSOLE MESSAGE]"RESET": counter stallo decrementata!\n\n");

    printf("\nIl filosofo %d STA MANGIANDO...\n", getpid());
    nanosleep(&tempo, NULL); //nano sleep dopo aver mangiato

    sem_post(forchetta[destra]); //libera il semaforo "di destra"
    printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());
    sem_post(forchetta[sinistra]); //libera il semaforo "di sinistra"
    printf("il filosofo con pid %d HA LASCIATO una forchetta\n", getpid());

  }

}

//INIZIO MAIN.
int main(int argc, char *argv[]) {

//CREAZIONE DELLA STRUCT SIGACTION.
    struct sigaction sa; //struttura sigaction
    memset(&sa, '\0', sizeof(struct sigaction));
    sa.sa_handler = f_handler; //chiamata funzione per interruzione con CTRL+C
    sigaction(SIGINT, &sa, NULL);


//ASSEGNAZIONE VALORI DEI FLAG E GESTIONE DEGLI ERRORI DI INPUT.
  if(argc < 2) { //Nessun argomento passato oltre ./"nome_file".
    printf(RED"\n\n[FATAL ERROR]"RESET": numero di filosofi non specificato!");
    exit(-1); //esce con codice -1
  } else if(argc<=2) { //Un solo argomento passato.
    printf(MAGENTA"\n\n[WARNING]"RESET": è stato inserito solo il numero di filosofi!");
    n_filosofi = atoi(argv[1]); //il numero di filosofi è il numero passato come argomento dopo "./nome_file"
    if(n_filosofi<=1) { //caso in cui l'input sia errato (<=0)
      printf(RED"\n\n[FATAL ERROR]"RESET": il numero di filosofi inseriti non è corretto!");
      exit(-1); //esce con il code -1
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d", n_filosofi);
  } else if(argc<=3) { //Solo due argomenti passati.
    printf(MAGENTA"\n\n[WARNING]"RESET": sono stati inseriti solo il numero di partecipanti ed il flag dello stallo!");
    n_filosofi = atoi(argv[1]); //il numero di filosofi è il numero passato come argomento dopo "./nome_file"
    flag_stallo = atoi(argv[2]); //il flag di stallo è il secondo numero passato come argomento
    if(n_filosofi<=1) {
      printf(RED"\n\n[FATAL ERROR]"RESET": il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d", n_filosofi, flag_stallo);
  } else if(argc<=4) { //Solo tre argomenti passati.
    printf(MAGENTA"\n\n[WARNING]"RESET": sono stati inseriti solo il numero di partecipanti, il flag dello stallo ed il flag per la soluzione allo stallo!");
    n_filosofi = atoi(argv[1]); //il numero di filosofi è il numero passato come argomento dopo "./nome_file"
    flag_stallo = atoi(argv[2]); //il flag di stallo è il secondo numero passato come argomento
    flag_soluzione_stallo = atoi(argv[3]); //il flag di soluzione allo stallo è il terzo numero passato come argomento
    if(n_filosofi<=1) {
      printf(RED"\n\n[FATAL ERROR]"RESET": il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d\nFlag della soluzione allo stallo: %d", n_filosofi, flag_stallo, flag_soluzione_stallo);
  } else if(argc<=5) { //Tutti gli argomenti passati correttamente.
    n_filosofi = atoi(argv[1]); //il numero di filosofi è il numero passato come argomento dopo "./nome_file"
    flag_stallo = atoi(argv[2]); //il flag di stallo è il secondo numero passato come argomento
    flag_soluzione_stallo = atoi(argv[3]); //il flag di soluzione allo stallo è il terzo numero passato come argomento
    flag_starvation = atoi(argv[4]); //il flag di starvation è il quarto numero passato come argomento
    if(n_filosofi<=1) {
      printf(RED"\n\n[FATAL ERROR]"RESET": il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d\nFlag della soluzione allo stallo: %d\nFlag della starvation: %d", n_filosofi, flag_stallo, flag_soluzione_stallo, flag_starvation);
  } else if(argc>5) { //troppi numeri passati come argomento --> presi solo i primi 4, gli altri sono tutti scartati
    printf(MAGENTA"\n\n[WARNING]"RESET": troppi elementi specificati, solo i primi 4 verranno considerati!");
    n_filosofi = atoi(argv[1]);
    flag_stallo = atoi(argv[2]);
    flag_soluzione_stallo = atoi(argv[3]);
    flag_starvation = atoi(argv[4]);
    if(n_filosofi<=1) {
      printf(RED"\n\n[FATAL ERROR]"RESET": il numero di filosofi inseriti non è corretto!");
      exit(-1);
    }
    printf("\n\nNumero di filosofi che partecipano alla cena: %d\nFlag dello stallo: %d\nFlag della soluzione allo stallo: %d\nFlag della starvation: %d", n_filosofi, flag_stallo, flag_soluzione_stallo, flag_starvation);
  }


  printf("\n\n\n Il Ristorante è aperto! Aspettando i filosofi...\n\n");
  sleep(5);

//STAMPE A VIDEO NEL CASO DI FLAG NULLI O NEGATIVI.
  if(n_filosofi<=1 || argv[1]==NULL) { //numero negativo/nullo/0 del numero di filosofi --> errore
    printf(RED"[FATAL ERROR]"RESET": numero di filosofi non corretto! chiusura in corso...");
    exit(0);
  }
  if (flag_stallo==0) { //numero negativo/nullo/0 del flag dello stallo --> stallo disattivato
    printf(MAGENTA"\n\n[WARNING]"RESET": flag dello stallo nulla, verrà disattivata!\n");
  } else if (flag_stallo < 0 || argv[2]==NULL) {
    printf(MAGENTA"\n\n[WARNING]"RESET": flag dello stallo negativa, verrà disattivata!\n");
    flag_stallo = 0;
  } else if(flag_stallo >1) {
    flag_stallo = 1;
  }
  if (flag_soluzione_stallo==0 ) { //numero negativo/nullo/0 del flag della soluzione allo stallo --> soluzione allo stallo disattivata
    printf(MAGENTA"\n\n[WARNING]"RESET": flag della soluzione allo stallo nulla, verrà disattivata!\n");
  } else if (flag_soluzione_stallo < 0|| argv[3]==NULL) {
    printf(MAGENTA"\n\n[WARNING]"RESET": flag della soluzione allo stallo negativa, verrà disattivata!\n");
    flag_soluzione_stallo=0;
  } else if(flag_soluzione_stallo>1) {
    flag_soluzione_stallo=1;
  }
  if (flag_starvation==0) { //numero negativo/nullo/0 del flag di starvation --> starvation disattivata
    printf(MAGENTA"\n\n[WARNING]"RESET": flag della starvation nulla, verrà disattivata!\n");
  } else if (flag_starvation < 0|| argv[4]==NULL) {
    printf(MAGENTA"\n\n[WARNING]"RESET": flag della starvazion negativa, verrà disattivata!\n");
    flag_starvation = 0;
  } else if(flag_starvation>1) {
    flag_starvation=1;
  }

//CREAZIONE DELLA PIPE PER LO STALLO
    if(pipe(pipefd) == -1) { //non c'è comunicazione corretta della stessa pipe tra tutti i child
      perror(RED"\n\n[FATAL ERROR]"RESET": i filosofi non riescono a mettersi d'accordo!\n");
      exit(EXIT_FAILURE); //uscita con fallimento
    }
    write(pipefd[1], &counter_stallo, sizeof(int)); //scrive il counter di stallo (inizializzata a 0 globalmente) nella pipe (usata in seguito per la funzione "mangia_con_stallo()")

//CREAZIONE DEI SEMAFORI (FORCHETTE).
  for(int i = 1; i <= n_filosofi; i++){ //ciclo per creare tanti semafori (forchette) quanti sono i filosofi a tavola
    snprintf(str, sizeof(str), "%d", i); //assegna "un nome" alla forchetta
    if((forchetta[i] = sem_open(str, O_CREAT, 1)) == SEM_FAILED){ //se il semaforo non viene creato correttamente il programma si chiude
      perror(BLU"\n\n[CONSOLE MESSAGE]"RESET": Manca una forchetta a tavola!\n");
      exit(EXIT_FAILURE); //uscita con fallimento
    }
  }

//CREAZIONE DEI PROCESSI CON FORK.
  pid_t filosofo[n_filosofi]; //array di filosofi di tipo pid_t. definito in questo modo perchè i filosofi saranno i child

  for(int i = 1; i <= n_filosofi; i++){
    filosofo[i] = fork(); //"sdoppia" il parent creando un nuovo processo figlio
    if(filosofo[i] < 0) { //se il child non viene creato correttamente il programma si chiude
  		 perror(RED"\n\n[FATAL ERROR]"RESET": uno dei filosofi non è potuto venire, la cena è rimandata!\n");
  		 exit(EXIT_FAILURE); //uscita con fallimento
    } else if(filosofo[i] == 0) { //Entro in uno dei child.
        printf("\nSi è unito un nuovo filosofo a tavola. Numero del filosofo: %d\n", getpid());

        //casistiche dei flag
        if(flag_stallo == 0 && flag_soluzione_stallo == 0 && flag_starvation == 0) { //solo filosofi
          mangia_solo_filosofi(forchetta, i);
        } else if (flag_stallo == 1 && flag_soluzione_stallo == 0 && flag_starvation == 0) { //stallo
          mangia_con_stallo(forchetta, i);
        } else if (flag_stallo == 1 && flag_soluzione_stallo == 1 && flag_starvation == 0) { //soluzione allo stallo
          mangia_senza_stallo(forchetta, i);
        } else if (flag_stallo == 1 && flag_soluzione_stallo == 0  && flag_starvation == 1) { //verifica se si verifica prima stallo o starvation
          mangia_stallo_e_starvation(forchetta, i);
        } else if (flag_stallo == 0 && flag_soluzione_stallo == 1 && flag_starvation == 0) { //mangia senza stallo
          mangia_senza_stallo(forchetta, i);
        } else if (flag_stallo == 0 && flag_soluzione_stallo == 1 && flag_starvation == 1) { //mangia con starvation attiva
          mangia_con_starvation(forchetta, i);
        } else if (flag_stallo == 1 && flag_soluzione_stallo == 1 && flag_starvation == 1) { //stallo risolto --> solo starvation
          mangia_con_starvation(forchetta, i);
        } else if (flag_stallo == 0 && flag_soluzione_stallo == 0 && flag_starvation == 1) { // mangia con starvation
          mangia_con_starvation(forchetta, i);
        }

      return 0; //Esco dal child quando ha finito.
    }
  }

  //sono nel parent

  for(int i = 1; i <= n_filosofi; i++) { //ciclo che serve per attendere che tutti i child termino l'esecuzione
    waitpid(filosofo[i], NULL, 0);
  }

  cancella_forchetta(n_filosofi); //cancella e unlinka tutti i semafori e cancella tutti i file descriptor
  printf("\n\nIl ristorante sta chiudendo, arrivederci!\n\n");
  exit(0); //esce con codice 0
} //fine dal main
