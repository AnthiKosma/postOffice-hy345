#include "libraries.h"
// MAIN
int main(int argc, char const * argv[]){
    //  initialise belt capacity
    if(argc != 4){
        fprintf(stderr, "ERROR: WRONG AMMOUNT OF ARGUMENTS\n");
        exit(-1);
    }
    int cussum = atoi(argv[1]);
    int empsum = atoi(argv[2]);
    int beltcap = atoi(argv[3]);

    if(cussum <= 0 || empsum <= 0 || beltcap <= 0){
        fprintf(stderr, "ERROR: ARGUMENTS MUST BE POSITIVE\n");
        exit(-1);
    }

    B = beltcap;
    C = cussum;
    C2 = C;

    belt_t intake, LOCAL, NATIONAL, INTERNATIONAL;
    //  Allocating intake belt
    intake.buffer = malloc(sizeof(parc_t *)*beltcap);
    if(intake.buffer == NULL){
        fprintf(stderr, "ERROR: Could not allocate intake belt");
        exit(-1);
    }
    sem_init(&intake.B, 0, B);
    sem_init(&intake.C, 0, 0);
    pthread_mutex_init(&intake.F, NULL);
    intake.read = intake.buffer;
    intake.write = intake.buffer;

    //  Allocating LOCAL belt
    LOCAL.buffer = malloc(sizeof(parc_t *)*beltcap);
    if(LOCAL.buffer == NULL){
        fprintf(stderr, "ERROR: Could not allocate LOCAL belt");
        exit(-1);
    }
    sem_init(&LOCAL.B, 0, B);
    sem_init(&LOCAL.C, 0, 0);
    pthread_mutex_init(&LOCAL.F, NULL);
    LOCAL.read = LOCAL.buffer;
    LOCAL.write = LOCAL.buffer;
    //  LOCAL postman
    pthread_t lp;
    pstIn_t inl;
    inl.dstBelt = &LOCAL;
    inl.pid = 0;
    if (pthread_create(&lp, NULL, postman, &inl) != 0) {
        fprintf(stderr, "ERROR: Could not make LOCAL postman thread");
        exit(-1);
    }

    //  Allocating NATIONAL belt
    NATIONAL.buffer = malloc(sizeof(parc_t *)*beltcap);
    if(NATIONAL.buffer == NULL){
        fprintf(stderr, "ERROR: Could not allocate NATIONAL belt");
        exit(-1);
    }
    sem_init(&NATIONAL.B, 0, B);
    sem_init(&NATIONAL.C, 0, 0);
    pthread_mutex_init(&NATIONAL.F, NULL);
    NATIONAL.read = NATIONAL.buffer;
    NATIONAL.write = NATIONAL.buffer;
    //  NATIONAL postman
    pthread_t np;
    pstIn_t inn;
    inn.dstBelt = &NATIONAL;
    inn.pid = 1;
    if (pthread_create(&np, NULL, postman, &inn) != 0) {
        fprintf(stderr, "ERROR: Could not make NATIONAL postman thread");
        exit(-1);
    }

    //  Allocating INTERNATIONAL belt
    INTERNATIONAL.buffer = malloc(sizeof(parc_t *)*beltcap);
    if(INTERNATIONAL.buffer == NULL){
        fprintf(stderr, "ERROR: Could not allocate INTERNATIONAL belt");
        exit(-1);
    }
    sem_init(&INTERNATIONAL.B, 0, B);
    sem_init(&INTERNATIONAL.C, 0, 0);
    pthread_mutex_init(&INTERNATIONAL.F, NULL);
    INTERNATIONAL.read = INTERNATIONAL.buffer;
    INTERNATIONAL.write = INTERNATIONAL.buffer;
    //  INTERNATIONAL postman
    pthread_t ip;
    pstIn_t ini;
    ini.dstBelt = &INTERNATIONAL;
    ini.pid = 2;
    if (pthread_create(&ip, NULL, postman, &ini) != 0) {
        fprintf(stderr, "ERROR: Could not make INTERNATIONAL postman thread");
        exit(-1);
    }

    //  Create employees
    pthread_t emp[empsum];
    empIn_t empInput[empsum];
    for(int i = 0; i<empsum; i++){
        empInput[i].eid = i;
        empInput[i].inBelt = &intake;
        empInput[i].LOCAL = &LOCAL;
        empInput[i].NATIONAL = &NATIONAL;
        empInput[i].INTERNATIONAL = &INTERNATIONAL;
        if (pthread_create(&emp[i], NULL, employee, &empInput[i]) != 0) {
            fprintf(stderr, "ERROR: Could not make employee thread");
            exit(-1);
        }
    }

    //  Create customers
    pthread_t cus[cussum];
    cusIn_t cusInput[cussum];
    for(int i = 0; i<cussum; i++){
        cusInput[i].inBelt = &intake;
        cusInput[i].cid = i;
        if (pthread_create(&cus[i], NULL, customer, &cusInput[i]) != 0) {
            fprintf(stderr, "ERROR: Could not make customer thread");
            exit(-1);
        }
    }

    // joining Customers
    for(int i=0; i<cussum; i++){
        pthread_join(cus[i], NULL);
    }
    
    // joining Employees
    for(int i = 0; i<empsum; i++) sem_post(&intake.C);
    for(int i=0; i<empsum; i++){
        pthread_join(emp[i], NULL);
    }

    // joining Postmen
    sem_post(&LOCAL.C);
    sem_post(&NATIONAL.C);
    sem_post(&INTERNATIONAL.C);

    pthread_join(ip, NULL);
    pthread_join(np, NULL);
    pthread_join(lp, NULL);

    //  Destroying and freeing
    sem_destroy(&INTERNATIONAL.B);
    sem_destroy(&INTERNATIONAL.C);
    pthread_mutex_destroy(&INTERNATIONAL.F);
    free(INTERNATIONAL.buffer);
    sem_destroy(&NATIONAL.B);
    sem_destroy(&NATIONAL.C);
    pthread_mutex_destroy(&NATIONAL.F);
    free(NATIONAL.buffer);
    sem_destroy(&LOCAL.B);
    sem_destroy(&LOCAL.C);
    pthread_mutex_destroy(&LOCAL.F);
    free(LOCAL.buffer);
    sem_destroy(&intake.B);
    sem_destroy(&intake.C);
    pthread_mutex_destroy(&intake.F);
    free(intake.buffer);

    pthread_mutex_destroy(&g);
    return 0;
}

// Function Implementation
//  Thread function implementations
void *customer(void *arg){
    cusIn_t *in = (cusIn_t *) arg;
    unsigned int seed = time(NULL) ^ in->cid;

    //  waits for t seconds (1 <= t <= 5) before creating their parcel
    int t = rand_r(&seed) % 5 + 1;
    sleep(t);

    //  creates a parcel with a random destination
    parc_t *p = malloc(sizeof(struct parcel));
    p->cid = in->cid;
    p->state = 0;
    p->dest = rand_r(&seed) % 3;
    sem_init(&(p->delivered), 0, 0);
    printf("[CUSTOMER %d] Created parcel(", p->cid);
    switch (p->dest){
        case 0:
            printf("LOCAL)\n");
            break;
        case 1:
            printf("NATIONAL)\n");
            break;
        case 2:
            printf("INTERNATIONAL)\n");
            break;
    }

    //  Waits until there is space on the intake belt
    sem_wait(&(in->inBelt->B));

    //  place parcel in the intake belt
    pthread_mutex_lock(&(in->inBelt->F));
    *(in->inBelt->write) = p;
    in->inBelt->write = (in->inBelt->write + 1 == in->inBelt->buffer + B) ?
                        in->inBelt->buffer : in->inBelt->write + 1;
    sem_post(&(in->inBelt->C));
    int cval;
    sem_getvalue(&(in->inBelt->C),&cval);
    printf("[CUSTOMER %d] Placed parcel on intake belt(%d/%d)\n",
        p->cid, cval, B);
    pthread_mutex_unlock(&(in->inBelt->F));

    //  Use semaphore for when the package is delivered
    sem_wait(&(p->delivered));

    //  destroy semaphore
    sem_destroy(&(p->delivered));

    //  free parcel
    free(p);
    
    return NULL;
}

void *employee(void *arg){
    empIn_t *belt = (empIn_t *) arg;
    unsigned int seed = time(NULL) ^ belt->eid;
    
    pthread_mutex_lock(&g);
    int c = C;
    pthread_mutex_unlock(&g);

    while(c>0){
        //  Monitor the intake belt for available parcels
        sem_wait(&(belt->inBelt->C));

        pthread_mutex_lock(&g);
        c = C;
        pthread_mutex_unlock(&g);
        if(c == 0) return NULL;
        
        //  Pick up a parcel
        pthread_mutex_lock(&(belt->inBelt->F));
        parc_t *p = *(belt->inBelt->read);
        belt->inBelt->read = (belt->inBelt->read + 1 == belt->inBelt->buffer + B) ?
                            belt->inBelt->buffer : belt->inBelt->read + 1;
        sem_post(&(belt->inBelt->B));
        printf("[EMPLOYEE %d] Picked up parcel from Customer %d (",
            belt->eid, p->cid);
        switch (p->dest){
        case 0:
            printf("LOCAL)\n");
            break;
        case 1:
            printf("NATIONAL)\n");
            break;
        case 2:
            printf("INTERNATIONAL)\n");
            break;
        }
        pthread_mutex_unlock(&(belt->inBelt->F));

        //  waits for t seconds (1 <= t <= 5) before creating their parcel
        int t = rand_r(&seed) % 5 + 1;
        sleep(t);

        //  read the parcel's destination
        int valc;
        switch(p->dest){
            case 0:
                //  Waits until there is space on the LOCAL belt
                sem_wait(&(belt->LOCAL->B));
                //  place parcel in the LOCAL belt
                pthread_mutex_lock(&(belt->LOCAL->F));
                *(belt->LOCAL->write) = p;
                belt->LOCAL->write = (belt->LOCAL->write + 1 == belt->LOCAL->buffer + B) ?
                                    belt->LOCAL->buffer : belt->LOCAL->write + 1;
                sem_post(&(belt->LOCAL->C));
                sem_getvalue(&(belt->LOCAL->C), &valc);
                printf("[EMPLOYEE %d] Placed parcel on LOCAL belt (%d/%d)\n",
                    belt->eid, valc, B);
                pthread_mutex_unlock(&(belt->LOCAL->F));
            break;
            case 1:
                //  Waits until there is space on the NATIONAL belt
                sem_wait(&(belt->NATIONAL->B));
                //  place parcel in the NATIONAL belt
                pthread_mutex_lock(&(belt->NATIONAL->F));
                *(belt->NATIONAL->write) = p;
                belt->NATIONAL->write = (belt->NATIONAL->write + 1 == belt->NATIONAL->buffer + B) ?
                                        belt->NATIONAL->buffer : belt->NATIONAL->write + 1;
                sem_post(&(belt->NATIONAL->C));
                sem_getvalue(&(belt->NATIONAL->C), &valc);
                printf("[EMPLOYEE %d] Placed parcel on NATIONAL belt (%d/%d)\n",
                    belt->eid, valc, B);
                pthread_mutex_unlock(&(belt->NATIONAL->F));
            break;
            case 2:
                //  Waits until there is space on the INTERNATIONAL belt
                sem_wait(&(belt->INTERNATIONAL->B));
                //  place parcel in the INTERNATIONAL belt
                pthread_mutex_lock(&(belt->INTERNATIONAL->F));
                *(belt->INTERNATIONAL->write) = p;
                belt->INTERNATIONAL->write = (belt->INTERNATIONAL->write + 1 == belt->INTERNATIONAL->buffer + B) ?
                                        belt->INTERNATIONAL->buffer : belt->INTERNATIONAL->write + 1;
                sem_post(&(belt->INTERNATIONAL->C));
                sem_getvalue(&(belt->INTERNATIONAL->C), &valc);
                printf("[EMPLOYEE %d] Placed parcel on INTERNATIONAL belt (%d/%d)\n",
                    belt->eid, valc, B);
                pthread_mutex_unlock(&(belt->INTERNATIONAL->F));
            break;
            default:
                fprintf(stderr, "ERROR: Non-existent destination");
                exit(-1);
                break;
        }
        pthread_mutex_lock(&g);
        c = C;
        pthread_mutex_unlock(&g);

    }
    return NULL;
}

void *postman(void *arg){
    pstIn_t *belt = (pstIn_t *)arg;
    unsigned int seed = time(NULL) ^ belt->pid;

    pthread_mutex_lock(&g);
    int c = C;
    pthread_mutex_unlock(&g);

    while(c>0){
        //  Monitor the assigned destination belt for parcels
        sem_wait(&(belt->dstBelt->C));

        pthread_mutex_lock(&g);
        c = C;
        pthread_mutex_unlock(&g);
        if(c == 0) return NULL;

        //  Pick up a parcel
        pthread_mutex_lock(&(belt->dstBelt->F));
        parc_t *p = *(belt->dstBelt->read);
        belt->dstBelt->read = (belt->dstBelt->read + 1 == belt->dstBelt->buffer + B) ?
                            belt->dstBelt->buffer : belt->dstBelt->read + 1;
        sem_post(&(belt->dstBelt->B));
        pthread_mutex_unlock(&(belt->dstBelt->F));

        //  Deliver parcel
        int t = rand_r(&seed) % 5 + 1;
        sleep(t);

        pthread_mutex_lock(&g);
        C-=1;
        int done = C2 - C;
        pthread_mutex_unlock(&g);

        printf("[POSTMAN %d] Delivered parcel from Customer %d (",
            belt->pid, p->cid);
        switch (p->dest){
        case 0:
            printf("LOCAL).");
            break;
        case 1:
            printf("NATIONAL).");
            break;
        case 2:
            printf("INTERNATIONAL).");
            break;
        }
        
        printf(" Total delivered: %d/%d\n", done, C2);

        //  Notify customer
        sem_post(&(p->delivered));

        pthread_mutex_lock(&g);
        c = C;
        pthread_mutex_unlock(&g);
    }
    return NULL;
}