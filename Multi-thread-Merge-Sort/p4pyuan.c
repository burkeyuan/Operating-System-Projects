#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*define the global variables stroring threads' id, mutex, condition, barrier, data and the number of digits*/
pthread_t sort[4096];
pthread_mutex_t mutex;
pthread_cond_t cond;
int barrier = 0;
int data[4096];
int num = 0;

/*define the struct used for transmit needed parameters*/
typedef struct {
    int nummerge;
    int left;
}NODE;

/*read the data from *.txt file, copy them to array data[] and get the number of digits*/
void read_data()
{  
    FILE *fp = fopen("indata.txt", "r");
    int i = 0;

    if(fp == NULL)
    { 
        printf("cannot open the file\n");
        exit(0);
    }

    while(!feof(fp))
    {
        fscanf(fp, "%d", &data[i]);
        i++;
        num++;   
    }

    num--;   
    printf("\n");
    fclose(fp);
    fp = NULL;
}

/*function merge(): according to the merge sort algorithm, sort power n of 2 of digits into a nondecreasing order*/
void *merge(void *arg) 
{
    NODE *sort = (NODE *)arg;

    int num_merge = sort->nummerge;
    int left = sort->left;
    int mid = left + num_merge/2 - 1;
   
     int a = left;
    int b = mid + 1;
    int cpy[num_merge];
    int ai = 0;
    
    while(a <= mid && b <= (left + num_merge - 1))
    {
        if(data[a] >= data[b])
        {
            cpy[ai++] = data[b++];
        }else
         {
             cpy[ai++] = data[a++];
         }
    }
    
    while(a <= mid)
    {
        cpy[ai++] = data[a++];
    } 
    
    while(b <= (left + num_merge - 1))
    {
        cpy[ai++] = data[b++];
    }  
    
    for(ai = 0; ai < num_merge; ai++)
    {
        data[left + ai] = cpy[ai];
    }

/*use mutex to prevent more than one threads accessing the barrier at the same time*/
    pthread_mutex_lock(&mutex);

    barrier--;
    //printf("barrier = %d\n", barrier);

/*if the barrier is over 0, the round has not ended so the current thread need to wait for other threads to complete*/    
    if(barrier > 0)
    {
        //printf("I am blocked\n");
        pthread_cond_wait(&cond, &mutex);
    }

/*if the barrier is no bigger than 0, all threads in current round have completed their jobs so there should be a broadcast*/    
    if(barrier <= 0)
    {    
        //printf("I am unblocked\n");
        pthread_cond_broadcast(&cond);     
    }

    pthread_mutex_unlock(&mutex);
    
}

/*function main()*/
void main()
{
    read_data();

    int k = 0, j = 0;
    int num_merge = 2;
    int num_thread = num/2;
    int num_now = 0;
    NODE m[2048];

/*initialize the mutex and cond*/
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

/*create threads round by round*/   
    while(num_merge <= num)
    {
        barrier = num_thread;

        for(j = 0; j < num_thread; j++)
        {  
            m[j].nummerge = num_merge;          
            m[j].left = j * num_merge;        
            pthread_create(&sort[num_now + j], NULL, (void *)merge, &m[j]);
        }
        
        num_now = num_thread + num_now;
        num_merge = num_merge * 2;
        num_thread = num_thread / 2;

/*prevent the main thread ending before the sort process*/
        if(barrier > 0)
        {
            pthread_cond_wait(&cond, &mutex);
        } 

        if(barrier <= 0)
        {    
            pthread_cond_broadcast(&cond);     
        }
    }

/*print the result of merge sort*/ 
    for(k = 0; k < num; k++)
    {
        printf("%d ", data[k]);
    }

    printf("\n");    

}
