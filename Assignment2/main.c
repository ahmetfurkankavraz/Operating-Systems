#define _XOPEN_SOURCE 1

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
In homework, I use 2 sempahores.
First is for synchronization between child processes.
And the other is for synchronization of parent and child processes.

Also, the shared memory space is used for data flow between processes.

SLEEP FUNCTION IS USED FOR SEEING THE SEMAPHORE IS RUNNING PROPERLY.
YOU CAN COMMENT SLEEP FUNCTIONS.

ALSO PROGRAM GIVES OUTPUT TO CONSOLE AND FILE.
FILE OUTPUTS ARE AS YOU WANT.
CONSOLE OUTPUTS ARE SEEING THE BREAKPOINTS OF THE CODE AND USAGE OF SEMAPHORES.

*/


void sem_signal(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = val;
    semaphore.sem_flg = 0;   
    semop(semid, &semaphore, 1);
}

void sem_wait(int semid, int val)
{
    struct sembuf semaphore;
    semaphore.sem_num = 0;
    semaphore.sem_op = (-1*val);
    semaphore.sem_flg = 0;  
    semop(semid, &semaphore, 1);
}

int KEYSEM;
int KEYSEM2;
int KEYSHM;
char *MEMKEY;

int main(int argc, char* argv[]){

    char cwd[256];

    //  get current working directory
    getcwd(cwd, 256);
    
    //  form keystring
    MEMKEY = malloc(strlen(cwd) + strlen(argv[0]) + 1);
    strcpy(MEMKEY, cwd);
    strcat(MEMKEY, argv[0]);

    /* FILE OPERATIONS */
    char* input_file;
    char* output_file;
    if (argc == 1){
        input_file = "input.txt";
        output_file = "output.txt";
    }
    else if (argc == 2){
        input_file = argv[1];
        output_file = "output.txt";
    }    
    else if (argc == 3){
        input_file = argv[1];
        output_file = argv[2];
    }   
    

    FILE* input_fp = freopen(input_file, "r", stdin);
    FILE* output_fp = fopen(output_file, "w");

    if (input_fp == NULL || output_fp == NULL){
        printf("FILE CAN NOT OPEN.\n");
        return 0;
    }

    int f, i;
    int *globalcp = NULL;
    int shmid = 0;

    KEYSEM = ftok(MEMKEY, 1);
    KEYSEM2 = ftok(MEMKEY, 2);
    KEYSHM = ftok(MEMKEY, 3);


    int sem_id2 = semget(KEYSEM2, 1, 0700|IPC_CREAT );
    semctl(sem_id2, 0, SETVAL, 0);


    /* reading values*/
    int M, n;
    scanf("%d\n", &M);
    scanf("%d\n", &n);

    printf("CREATING SHARED MEMORY SPACE\n");
    shmid = shmget(KEYSHM, (4+ n*2)*sizeof(int), 0700|IPC_CREAT);

    globalcp = (int*)shmat(shmid, 0, 0);
    globalcp[0] = n;
    globalcp[1] = M;

    printf("APPENDING A VALUES INTO SHARED MEMORY\n");
    for (int k=0; k < n; k++){
        scanf("%d ", &globalcp[k+4]);
    }

    shmdt(globalcp);

    
    printf("CHILD PROCESS CREATION\n");
    for (i = 0; i < 2; ++i)
    {
        f = fork();
        if (f < 0)
        {
            printf("FORK error...\n");
            exit(1);
        }
        if (f == 0)
            break;
    }


    if (f > 0){ // PARENT

        printf("PARENT:  INSIDE PARENT PROCESS\n");
        

        // SYNCHRONIZATION FOR PARENT PROCESS
        // WHICH SHOULD IS FOR WAITING THE TERMINATION OF CHILD PROCESSES
        //int sem_id2 = semget(KEYSEM2, 1, 0700|IPC_CREAT );
        //semctl(sem_id2, 0, SETVAL, 0);


        int sem_id = semget(KEYSEM, 1, 0);
        semctl(sem_id, 0, SETVAL, 0);


        printf("PARENT:  WAITING CHILD PROCESSES...\n");
        sem_wait(sem_id2, 2);
        printf("PARENT:  CONTINUE...\n");


        shmid = shmget(KEYSHM, (4+ n*2)*sizeof(int), 0);
        globalcp = (int*)shmat(shmid, 0, 0);
        


        printf("PARENT:  WRITING ALL VALUES INTO OUTPUT FILE\n");
        fprintf(output_fp, "%d\n", globalcp[1]); // M

        fprintf(output_fp, "%d\n", globalcp[0]); // n

        for (int k = 0; k < globalcp[0]; k++){ // array A
            fprintf(output_fp, "%d", globalcp[4+k]);
            if (k == globalcp[0]-1)
                fprintf(output_fp, "\n");
            else
                fprintf(output_fp, " ");
        }

        fprintf(output_fp, "%d\n", globalcp[2]); // x

        for (int k = 0; k < globalcp[2]; k++){ // array B
            fprintf(output_fp, "%d", globalcp[4+globalcp[0]+k]);
            if (k == globalcp[2]-1)
                fprintf(output_fp, "\n");
            else
                fprintf(output_fp, " ");
        }

        fprintf(output_fp, "%d\n", globalcp[3]); // x

        for (int k = 0; k < globalcp[3]; k++){ // array B
            fprintf(output_fp, "%d", globalcp[4+globalcp[0]+globalcp[2]+k]);
            if (k != globalcp[3]-1)
                fprintf(output_fp, " ");
        }

        shmdt(globalcp);

        fclose(input_fp);
        fclose(output_fp);
        printf("PARENT PROCESS EXIT\n");

        semctl(sem_id, 0, IPC_RMID, 0);
        semctl(sem_id2, 0, IPC_RMID, 0);
        shmctl(shmid, IPC_RMID, 0);
        
        exit(0);        
        
    }
    else {

        shmid = shmget(KEYSHM, (4+ n*2)*sizeof(int), 0);
        globalcp = (int*)shmat(shmid, 0, 0);
        //int sem_id2 = semget(KEYSEM2, 1, 0);

        // CHILD 1
        if (i == 0){
            printf("CHILD 1:  INSIDE CHILD 1 PROCESS\n");
            
            // Synchronization Semaphore Creation
            int sem_id = semget(KEYSEM, 1, 0700|IPC_CREAT );
            semctl(sem_id, 0, SETVAL, 0);
            
            int x=0;

            printf("CHILD 1:  CALCULATING x VALUE\n");
            for (int k=0; k < globalcp[0]; k++){
                if (globalcp[k+4] <= globalcp[1]){
                    x++;
                }
            }
            globalcp[2] = x;
            

            /*SLEEPING FOR BETTER USAGE OF SEMAPHORE*/

            sleep(2);

            printf("CHILD 1:  SYNCHRONIZATION SIGNAL FOR CHILD 2\n");
            //Synchronization
            sem_signal(sem_id, 1);

            sleep(2);

            int k=0;
            int b_index = 0;
            printf("CHILD 1:  APPENDING B VALUES INTO SHARED MEMORY\n");
            while (k < globalcp[0]){
                
                if (globalcp[k+4] <= globalcp[1]){
                    globalcp[4+globalcp[0]+b_index] = globalcp[k+4];
                    b_index++;
                }
                k++;
            }
            
        }

        // CHILD 2
        else if (i == 1){
            
            printf("CHILD 2:  INSIDE CHILD 2 PROCESS\n");
            //int sem_id2 = semget(KEYSEM2, 1, 0);


            //Synchronization
            int sem_id = semget(KEYSEM, 1, 0 );

            int y = 0;
            printf("CHILD 2:  CALCULATING y VALUE\n");
            for (int k=0; k < globalcp[0]; k++){
                if (globalcp[k+4] > globalcp[1]){
                    y++;
                }
            }
            globalcp[3] = y;

            printf("CHILD 2:  CHILD 2 IS WAITING CHILD 1 \n");
            sem_wait(sem_id, 1);
            printf("CHILD 2:  CONTINUE... \n");

            printf("CHILD 2:  APPENDING C VALUES INTO SHARED MEMORY\n");
            int k=0;
            int c_index = 0;
            while (k < globalcp[0]){
                
                if (globalcp[k+4] > globalcp[1]){
                    globalcp[4+globalcp[0]+globalcp[2]+c_index] = globalcp[k+4];
                    c_index++;
                }
                k++;
            }
            
        }
        shmdt(globalcp);

        sem_signal(sem_id2, 1);
        printf("FINISH CHILD PROCESS %d\n", i+1);
        exit(0);

    }
    return 0;
}
