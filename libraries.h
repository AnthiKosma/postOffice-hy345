#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

typedef enum destination{ LOCAL = 0, NATIONAL = 1, INTERNATIONAL = 2 } dest_t;
typedef enum parcelState{ WAITING = 0, IN_BELT = 1} st_t;

typedef struct parcel{
    dest_t dest;    // destination
    st_t state;     // parcel's state;
    sem_t delivered;// semaphore per customer/parcel for when it is delivered
    int cid;        // customer id
} parc_t;

typedef struct Belts{
    parc_t **buffer; // belt buffer
    sem_t B;        // available belt slots semaphore
    sem_t C;        // available item count semaphore
    pthread_mutex_t F;  // read write mutex
    //  read/write positions
    parc_t **read;
    parc_t **write;
} belt_t;

typedef struct customerInput{
    int cid;
    belt_t *inBelt;
} cusIn_t;

typedef struct employeeInput{
    int eid;
    belt_t *inBelt;
    belt_t *LOCAL;
    belt_t *NATIONAL;
    belt_t *INTERNATIONAL;
} empIn_t;

typedef struct postmanInput{
    int pid;
    belt_t *dstBelt;
} pstIn_t;

//  Global variables and structures
int B; // Belt capacity
int C; // Parcel count / Customer count
int C2; 

pthread_mutex_t g = PTHREAD_MUTEX_INITIALIZER;

//  Functions
//  Thread functions
void *customer(void *);
void *employee(void *);
void *postman(void *);