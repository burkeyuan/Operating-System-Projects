#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//define the ids of threads and global variables
    
pthread_t thread[11];
pthread_mutex_t mut;
int data[9][9];
int a = 0, b = 0, i = 0, j = 0, k = 0, m = 0, err = 0, err1 = 0;

//read the digits from the sudoku.txt file

void read_data()
{  
    FILE *fp = fopen("sudoku.txt", "r");
    
    if(fp == NULL)
    { 
        printf("cannot open the file\n");
        exit(0);
    }  
    
    for(i = 0; i < 9; i++)
    {
        for(j = 0; j < 9; j++)
        {   
            fscanf(fp, "%d", &data[i][j]);   
        }
    } 
 
    printf("\n");
    fclose(fp);
    fp = NULL;
}

//thread1: check the digits row by row

void *thread1()
{
    for(a = 0; a < 9; a++)
    { 
        pthread_mutex_lock(&mut);
        err = 0;  
        int flag[10] = {0,0,0,0,0,0,0,0,0,0};
        
        for(j = 0; j < 9; j++)
        {   
            flag[data[a][j]] = 1;
        }
        for(k = 1; k < 10; k++)
        {
            if(flag[k] == 0)
            {
                err++;
                if(err == 1)
                {
                printf("row = %d, Missing digit = ", a+1);
                }
                printf("%d ", k);
            }        
        }
        if(err != 0)
        {
            printf("\n");
            err1++;
        }
        pthread_mutex_unlock(&mut); 
    }
    pthread_exit(NULL);   
}

//thread2: check the digits column by column

void *thread2()
{
    for(b = 0; b < 9; b++)
    {   
        pthread_mutex_lock(&mut);
        err = 0; 
        int flag[10] = {0,0,0,0,0,0,0,0,0,0};
 
        for(j = 0; j < 9; j++)
        {   
            flag[data[j][b]] = 1;
        }
        for(k = 1; k < 10; k++)
        {
            if(flag[k] == 0)
            {
                err++;
                if(err == 1)
                {
                printf("column = %d, Missing digit = ", b+1);
                }
                printf("%d ", k);
            }        
        }
        if(err != 0)
        {
            printf("\n");
            err1++;
        }
        pthread_mutex_unlock(&mut);  
    }
    pthread_exit(NULL);   
}

//function: check if there are missing numbers in a 3 * 3 square

void checksquare(int row, int column)
{
    i = 0, j =0, k = 0, m = 0;
    err = 0;
    int flag[10] = {0,0,0,0,0,0,0,0,0,0};

    for(i = row; i < row + 3; i++)
    {   
        for(j = column; j < column + 3; j++)
        {   
            flag[data[i][j]] = 1;
        }        
    }
    for(k = 1; k < 10; k++)
    {
        if(flag[k] == 0)
        {
            err++;
            if(err == 1)
            {
                printf("3x3 subgrid of row %d..%d and column %d..%d, Missing digit = ", row + 1, row + 3, column + 1, column + 3);
            }
            printf("%d ", k);
         }        
    }
    if(err != 0)
    {
       printf("\n");
       err1++;
    } 
}

//thread3 - 11: check the nine squares

void *thread3()
{
    pthread_mutex_lock(&mut);
    checksquare(0, 0);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}

void *thread4()
{
    pthread_mutex_lock(&mut);   
    checksquare(0, 3);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}

void *thread5()
{
    pthread_mutex_lock(&mut);   
    checksquare(0, 6);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}
void thread6()
{
    pthread_mutex_lock(&mut);   
    checksquare(3, 0);   
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}
void *thread7()
{
    pthread_mutex_lock(&mut);   
    checksquare(3, 3);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}
void *thread8()
{
    pthread_mutex_lock(&mut);   
    checksquare(3, 6);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}
void *thread9()
{
    pthread_mutex_lock(&mut);   
    checksquare(6, 0);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}
void *thread10()
{
    pthread_mutex_lock(&mut);   
    checksquare(6, 3);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}

void *thread11()
{
    pthread_mutex_lock(&mut);   
    checksquare(6, 6);
    pthread_mutex_unlock(&mut); 
    pthread_exit(NULL); 
}

//create the threads

void create_thread(void)
{
    int i = 0; 
    int symbol[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    memset(&thread, 0, sizeof(thread));
    
    symbol[0] = pthread_create(&thread[0], NULL,(void *) thread1, NULL);   
    symbol[1] = pthread_create(&thread[1], NULL,(void *) thread2, NULL); 
    symbol[2] = pthread_create(&thread[2], NULL,(void *) thread3, NULL);  
    symbol[3] = pthread_create(&thread[3], NULL,(void *) thread4, NULL); 
    symbol[4] = pthread_create(&thread[4], NULL,(void *) thread5, NULL); 
    symbol[5] = pthread_create(&thread[5], NULL,(void *) thread6, NULL);  
    symbol[6] = pthread_create(&thread[6], NULL,(void *) thread7, NULL);  
    symbol[7] = pthread_create(&thread[7], NULL,(void *) thread8, NULL);  
    symbol[8] = pthread_create(&thread[8], NULL,(void *) thread9, NULL);  
    symbol[9] = pthread_create(&thread[9], NULL,(void *) thread10, NULL);  
    symbol[10] = pthread_create(&thread[10], NULL,(void *) thread11, NULL);  
    
    for(i = 1; i < 11; i++)
    { 
        if(symbol[i] != 0)
        {
             printf("Failure: create thread 1\n");printf("%d\n", symbol[i]); 
        }
    }
}

//wait the threads to terminate

void wait_thread()
{
    int i = 0;
    for(i = 0; i < 11; i++)
    {
        if(thread[i] != 0)
        {
            pthread_join(thread[i], NULL);
        }
    }
}

//the main function

void main()
{
    int i =0, j =0;
    pthread_mutex_init(&mut, NULL);
    read_data();
    create_thread();
    wait_thread();
    if(err1 == 0)
    {
        printf("The Sudoku puzzle is valid\n");
    }       
}
