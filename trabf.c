//Libraries used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


//Global variables

FILE *fptr , *ler, *target;                                  //Files to be created       
char name[20];                                               //name of log file
int  NTHREADS_L, NTHREADS_E;                                 //number of threads for both reading and writing   
int number_l, number_e;                                      //number of reads and writes per thread
int shared = 0;                                              // shared variable
int leit = 0, escr = 0, esperaEscrita = 0;                   //number of readers reading, number of writers writin and variable giving priority to writers 

pthread_mutex_t mutex;                                       //mutual exclusion for critical sections
pthread_cond_t cond_leit, cond_escr;                        //conditional variables for reading and writing

//Prototypes
void initLeitura(int tid);
void fimLeitura(int tid);
void initEscrita(int tid);
void fimEscrita(int tid);
void *leitor (void *arg);
void *escritor (void *arg);
    

int main() { 
    //Take number of threads for both reading and writing from the user 
    printf("number of threads leitoras and number of threads escritoras\n");
    scanf("%d %d", &NTHREADS_L, &NTHREADS_E);
   
    //Take number reading and writings for the threads taken above (NTHREADS_L and NTHREADS_E)
    printf("number of leituras and number of escritas\n");
    scanf("%d %d", &number_l, &number_e);

    //Take log file name 
    printf("Enter log file name\n");
    scanf("%s", name);

    //create log file
    fptr = fopen(name, "w+");
    if(fptr == NULL){
        printf ("File cannot be opened");
        return 1;
    }

    //create auxiliary program file
    target = fopen("auxiliar.py", "w+");
    if(target == NULL){
        printf ("File cannot be opened");
        return 1;
    }

    //code for auxiliary program
    fprintf(target,"esc=0\nleit=0\n\n");
    fprintf(target, "def initLeitura(id):\n\tif(esc==0):\n\t\tglobal leit\n\t\tleit+=1\n\telse:\n\t\tprint(\"Error na  \" + str(id))\n\t\treturn -1\n\n");
    fprintf(target, "def fimLeitura(id):\n\tglobal leit\n\tleit-=1\n\n");
    fprintf(target, "def initEscrita(id):\n\tif(leit==0):\n\t\tglobal esc\n\t\tesc+=1\n\telse:\n\t\tprint(\"Error na  \" + str(id))\n\t\treturn -1\n\n");
    fprintf(target, "def fimEscrita(id):\n\tglobal esc\n\tesc-=1\n\n");

    //ids of threads
    pthread_t *tids1;
    pthread_t *tids2;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_leit, NULL);
    pthread_cond_init(&cond_escr, NULL);

    int *tid1, *tid2; //pointers to take arguments of threads

    //allocating memory to ids
    tids1 = malloc(sizeof(pthread_t) * (NTHREADS_L)); if(!tids1) exit(-1);
    tids2 = malloc(sizeof(pthread_t) * (NTHREADS_E)); if(!tids2) exit(-1);
    
    //creating threads and running ireader function
    for(int i = 0; i < NTHREADS_L; i++) {
        tid1 = malloc(sizeof(int));  if(!tid1) exit(-1);
        *tid1 = i;
        pthread_create(&tids1[i], NULL, leitor, (void * ) tid1);
    }
    //creating threads and running writer function
    for(int i = 0; i < NTHREADS_E; i++) {
        tid2 = malloc(sizeof(int));  if(!tid2) exit(-1);
        *tid2 = i;
        pthread_create(&tids2[i], NULL,escritor, (void * ) tid2);
    }
    //joining threads
    for (int i=0; i<NTHREADS_L; i++) {
        if (pthread_join(tids1[i], NULL)) {
             printf("--ERRO: pthread_join() \n"); exit(-1); 
            } 
    }
    //joining threads
    for (int i=0; i<NTHREADS_E; i++) {
        if (pthread_join(tids2[i], NULL)) {
             printf("--ERRO: pthread_join() \n"); exit(-1); 
            } 
    }

    fprintf(target,"print(\"successful,no errors.\")");
    //freeing
    free(tids1);
    free(tids2);
    free(tid1);
    free(tid2);
    pthread_exit(NULL);

 return 0;
}


//////FUNCTIONS

void initLeitura(int tid) {
    pthread_mutex_lock(&mutex);                                   //Guarantee exclusive access to the reader
    char nome[20];                                                //to take name of the file to be generated after reading
    while(escr > 0 || esperaEscrita > 0) { 
        pthread_cond_wait(&cond_leit, &mutex);
    }
    leit++;
    fprintf(target,"initLeitura(%d)\n",  tid);                    //write executed function in auxiliary program
    
    sprintf(nome, "%d", tid);                                     //convert int to string 
    strcat(nome, ".txt");                                         //concatenate nome to .txt so we have something of the form nome.txt
    ler = fopen(nome, "w");                                       //create file nome.txt
    if(ler == NULL){
        printf ("File cannot be opened");
        exit(-1);
    }
    fprintf(ler,"%d\n", shared);                                  //write in the file 
    pthread_mutex_unlock(&mutex);                                 //free lock
}
void fimLeitura(int tid) {
    pthread_mutex_lock(&mutex);
    leit--;
    fprintf(target, "fimLeitura(%d)\n", tid);                     //write executed function in auxiliary program 
    if(leit == 0) {
        pthread_cond_signal(&cond_escr);
    }
    pthread_mutex_unlock(&mutex);
}
void initEscrita(int tid) {
    pthread_mutex_lock(&mutex);
    esperaEscrita++;
    while((leit > 0 || escr > 0)) {
        pthread_cond_wait(&cond_escr, &mutex);
    }
    esperaEscrita--;
    escr++;
    fprintf(target, "initEscrita(%d)\n", tid);                     //write executed function in auxiliary program 
    shared = tid;
    pthread_mutex_unlock(&mutex);

}
void fimEscrita(int tid) {
    pthread_mutex_lock(&mutex);
    escr--;
    fprintf(target, "fimEscrita(%d)\n", tid);                       //write executed function in auxiliary program 
    pthread_cond_signal(&cond_escr);
    pthread_cond_broadcast(&cond_leit);
    pthread_mutex_unlock(&mutex);
}
void * leitor (void *arg) {
    int tid = * (int *) arg;
    int a = 0;    
    while(a <= number_l){                                    //Repeat number_l of times 
        initLeitura(tid);                                    //initialize reading 
        fprintf(fptr,"initLeitura(%d)\n",  tid);             //write executed function in log file 
        fimLeitura(tid);                                     //End reading
        fprintf(fptr, "fimLeitura(%d)\n", tid);              //writing executed function in log file
        a++;
    }
    pthread_exit(NULL);
}
void *escritor (void *arg) {
    int tid = * (int *) arg;
    int a = 0;
    while(a <= number_e) {
        initEscrita(tid);                                   //initialize writing
        fprintf(fptr, "initEscrita(%d)\n", tid);            //write executed function in log file
        fimEscrita(tid);                                    //End writing
        fprintf(fptr, "fimEscrita(%d)\n", tid);             //write executed function in log file
        a++;
    }
    pthread_exit(NULL);
}
