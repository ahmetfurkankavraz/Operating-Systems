/*
Author: Ahmet Furkan Kavraz
Number: 150190024
*/

/*
The code has two synchronization and one mutual exclusion semaphore.
And also one shared memory for publishing data between processes.
The publishing data is news-source process is for this program. It is changeable.
*/

#define _XOPEN_SOURCE 1

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//default semaphore functions
//received from recit slides
void sem_wait(int semid, int val){ // p()
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = (-1*val);
    semaphore.sem_flg = 0;  
    semop(semid, &semaphore, 1);
}
void sem_signal(int semid, int val){ // v()
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 0;   
    semop(semid, &semaphore, 1);
}


int m; //number of news sources
int n; //number of subscribers

int* globalcp;
int received_data;

//key of semaphores for creating them
int KEYSEM;
int KEYSEM2;
int KEYSEM3;
int KEYSHM;
char *MEMKEY;


//semaphores are used in functions, so they are defined as global variables
int publish_mutex; 
int read_sem;
int publisher_sem;

void read_news(){

    // If there is no published data, it will wait. (synchronization)
    //printf("Process %d waits for reading the published data.\n", getpid());
    sem_wait(read_sem, 1);

    // Every subscriber call the function only one time.
    if (received_data != *globalcp){

        //reading data 
        printf("Process %d received data from process %d.\n", getpid(), *globalcp);
        received_data = *globalcp;

        // Signal for the news-source
        sem_signal(publisher_sem, 1);
    }
    else {
        sem_signal(read_sem, 1); //incrementing because it will already reach data, other subscribers should reach the data.
        //printf("The data is received before\n"); // you can remove the comment but the output will be terrible. 
    }

}

void publish(){

    printf("Process %d waits for publishing its data.\n", getpid());

    // Wait previous news-source
    sem_wait(publish_mutex, 1); //mutex

    //Publishing the data
    int data_publisher_id = getpid();
    printf("Process %d published data.\n", data_publisher_id);
    *globalcp = data_publisher_id;
    
    // Signal for subscribers
    sem_signal(read_sem, n); //increment semaphore n times for n different subscriber.

    // Waits until the data is read by each subscriber.
    sem_wait(publisher_sem, n);
    sem_signal(publish_mutex, 1); // and giving signal for other news-source.
}



int main(){
    
    char cwd[256];

    //  get current working directory
    getcwd(cwd, 256);
    
    //  form keystring
    MEMKEY = malloc(strlen(cwd) + 1);
    strcpy(MEMKEY, cwd);

    KEYSEM = ftok(MEMKEY, 1);
    KEYSEM2 = ftok(MEMKEY, 2);
    KEYSEM3 = ftok(MEMKEY, 3);
    KEYSHM = ftok(MEMKEY, 4);


    //for mutual exclusion when changing the common datas
    publish_mutex = semget(KEYSEM, 1, 0700|IPC_CREAT ); 
    semctl(publish_mutex, 0, SETVAL, 1);
    
    //for synchronization
    read_sem = semget(KEYSEM2, 1, 0700|IPC_CREAT );
    semctl(read_sem, 0, SETVAL, 0);

    publisher_sem = semget(KEYSEM3, 1, 0700|IPC_CREAT );
    semctl(publisher_sem, 0, SETVAL, 0);

    printf("CREATING SHARED MEMORY SPACE\n");
    int shmid = shmget(KEYSHM, sizeof(int), 0700|IPC_CREAT);
    globalcp = (int*)shmat(shmid, 0, 0);


    //getting the value for m and n
    printf("Please give a value for number of news source: ");
    scanf("%d", &m);

    printf("Please give a value for number of subscribers: ");
    scanf("%d", &n);
    printf("\n");

    // Creating m different news source and n different subscribers
    int i;
    for (i = 0; i < m+n-1; i++){
        int j = fork();

        if (j == 0){
            break;
        }   
    }

    // if it is subscriber:
    if (i < n){
        printf("The process %d is a subscriber process.\n", getpid());

        // calling too many read_news function for ending program properly, otherwise program may not end
        for (int k = 0; k < m; k++){  
            received_data = -1; // every loop received data should be cleared.
            read_news(); 
            //read_news(); // what happens if we call second time? 
        }
        exit(0);
    }
    // if it is news-source:
    else {
        printf("The process %d is a publisher process.\n", getpid());
        sleep(2);
        publish();
        exit(0);
    }
    semctl(publish_mutex, 0, IPC_RMID, 0);
    semctl(read_sem, IPC_RMID, 0);
    shmctl(shmid, IPC_RMID, 0);
    return 0;


}