#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>


void* child(void*);
void* adult(void*);
void initSynch(void);

pthread_cond_t boatInOahu;
pthread_cond_t boatInMol;
pthread_cond_t lastAdultCrossed;
pthread_cond_t showedUpInOahu;
pthread_cond_t finishedTransporting;
pthread_cond_t transportingChildren;

pthread_mutex_t loadBoat;
pthread_mutex_t numChildrenInBoatLock;
pthread_mutex_t inOahuChildLock;
pthread_mutex_t inMolChildLock;
pthread_mutex_t inOahuAdultLock;
pthread_mutex_t inMolAdultLock;
pthread_mutex_t finished;

int childrenInOahu = 0;
int adultsInOahu = 0;
int childrenInMol = 0;
int adultsInMol = 0;

int numChildrenInBoat = 0;
bool boatIsInOahu = true;

sem_t* numChild;
sem_t* numAdult;


int main(int args, char *argv[]){
	initSynch();
	printf("LET'S TRANSPORT PEOPLE FROM OAHU TO MOLOKAI!!!!\n\n\n");
	fflush(stdout);
	const int numchildren = atoi(argv[1]);
	const int numadults = atoi(argv[2]);
	pthread_t childthreads[numchildren];
	pthread_t adultthreads[numadults];
	
	
	numChild = sem_open("num_child", O_CREAT|O_EXCL, 0466, 0);
  	while (numChild==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore numChild already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("num_child");
	      numChild = sem_open("num_child", O_CREAT|O_EXCL, 0466, 10);
	    }
	    else {
	      printf("semaphore could not be opened, error # %d\n", errno);
	      fflush(stdout);
	      exit(1);
	    }
  	}

  	numAdult = sem_open("num_adult", O_CREAT|O_EXCL, 0466, 0);
  	while (numAdult==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore numAdult already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("num_adult");
	      numAdult = sem_open("num_adult", O_CREAT|O_EXCL, 0466, 10);
	    }
	    else {
	      printf("semaphore could not be opened, error # %d\n", errno);
	      fflush(stdout);
	      exit(1);
	    }
  	}

	int err;
	for (int i = 0; i < numchildren; i++){
		pthread_create(&childthreads[i], NULL, child, NULL);
		
		err = sem_wait(numChild);
		// sleep(1);
	}

	for (int i = 0; i < numadults; i++){
		pthread_create(&adultthreads[i], NULL, adult, NULL);
		err = sem_wait(numAdult);
		// sleep(1);
	}

  	pthread_cond_broadcast(&showedUpInOahu);

  	pthread_cond_wait(&finishedTransporting, &finished);
  	sem_close(numChild);
  	sem_close(numAdult);
  	sem_unlink("num_child");
  	sem_unlink("num_adult");
  	exit(1);

}

void* child(void* args){
	printf("Child arrived to OAHU\n");
	fflush(stdout);
	sem_post(numChild);	
	bool inOahu = true;

	pthread_mutex_lock(&inOahuChildLock);
	childrenInOahu++;
	pthread_cond_wait(&showedUpInOahu, &inOahuChildLock);
	pthread_mutex_unlock(&inOahuChildLock);

	while (true){
		// printf("in loop\n");
		// fflush(stdout);
		if (inOahu) {
			pthread_mutex_lock(&inOahuChildLock);
			if (childrenInOahu > 1){
				pthread_mutex_unlock(&inOahuChildLock);
				printf("++++++in if statement+++++\n");
				fflush(stdout);
				pthread_mutex_lock(&loadBoat);
				printf("++++++Locked boat+++++\n");
				fflush(stdout);
				if(boatIsInOahu) {
					numChildrenInBoat++;
					if (numChildrenInBoat == 1){
						printf("Child boarded boat in OAHU\n");
						fflush(stdout);
						pthread_mutex_unlock(&loadBoat);
					}
					else{
						printf("Child boarded boat in OAHU\n");
						fflush(stdout);
						pthread_mutex_lock(&inOahuChildLock);
						pthread_mutex_lock(&inMolChildLock);
						childrenInOahu -= 2;
						childrenInMol += 2;
						numChildrenInBoat = 0;
						pthread_mutex_unlock(&inOahuChildLock);
						pthread_mutex_unlock(&inMolChildLock);
						pthread_mutex_unlock(&loadBoat);

					}
					inOahu = false;
					// pthread_mutex_unlock(&numChildrenInBoatLock);
					printf("Child arrived in MOL\n");
					fflush(stdout);
					pthread_cond_broadcast(&boatInMol);
					boatIsInOahu = false;
				}
				else {
					pthread_mutex_unlock(&loadBoat);
				}
				// pthread_cond_wait(&boatInOahu, &loadBoat);
				// printf("++++++Boat in Oahu condition passed+++++\n");
				// fflush(stdout);
				// // pthread_mutex_lock(&numChildrenInBoatLock);
				

			}
			else{
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_cond_broadcast(&transportingChildren);
				pthread_mutex_lock(&inOahuAdultLock);
				if (adultsInOahu == 0){
					pthread_mutex_unlock(&inOahuAdultLock);
					pthread_cond_wait(&boatInOahu, &loadBoat);
					pthread_mutex_lock(&loadBoat);
					inOahu = false;
					childrenInMol++;
					childrenInOahu--;
					pthread_cond_broadcast(&boatInMol);
					pthread_cond_signal(&finishedTransporting);
					pthread_mutex_unlock(&loadBoat);
				}
			}
		}
		else{
			pthread_cond_wait(&boatInMol, &loadBoat);
			pthread_mutex_lock(&inOahuChildLock);
			pthread_mutex_lock(&inMolChildLock);
			childrenInOahu++;
			childrenInMol--;
			inOahu = true;
			printf("Child arrived in OAHU\n");
			fflush(stdout);
			pthread_cond_broadcast(&boatInOahu);
			pthread_mutex_unlock(&inOahuChildLock);
			pthread_mutex_unlock(&inMolChildLock);
		}
		
	}


}

void* adult(void* args){
	printf("Adult arrived to OAHU\n");
	fflush(stdout);
	sem_post(numAdult);
	pthread_mutex_lock(&inOahuAdultLock);
	pthread_cond_wait(&showedUpInOahu, &inOahuAdultLock);
	// printf("hello1");
	// fflush(stdout);
	pthread_mutex_unlock(&inOahuAdultLock);
	// printf("Everyone has arrived");
	// fflush(stdout);
	bool inOahu = true;
	pthread_mutex_lock(&inOahuAdultLock);
	adultsInOahu++;
	pthread_mutex_unlock(&inOahuAdultLock);
	pthread_mutex_lock(&inOahuAdultLock);
	pthread_cond_wait(&transportingChildren, &inOahuAdultLock);
	printf("Adult started to cross\n");
	fflush(stdout);
	pthread_mutex_unlock(&inOahuAdultLock);
	pthread_mutex_lock(&loadBoat);
	pthread_cond_wait(&boatInOahu, &loadBoat);
	printf("Adult %s arrived in MOL\n", (char*) args);
	fflush(stdout);
	inOahu = false;
	adultsInOahu--;
	adultsInMol++;
	pthread_mutex_unlock(&loadBoat);
	pthread_cond_signal(&boatInMol);
	return (void*) 0;
}

void initSynch() {
	pthread_mutex_init(&loadBoat, NULL);
	pthread_mutex_init(&inOahuChildLock, NULL);
	pthread_mutex_init(&inMolChildLock, NULL);
	pthread_mutex_init(&inOahuAdultLock, NULL);
	pthread_mutex_init(&inMolAdultLock, NULL);
	pthread_mutex_init(&numChildrenInBoatLock, NULL);
	pthread_mutex_init(&finished, NULL);

  	pthread_cond_init(&boatInOahu, NULL);
  	pthread_cond_init(&boatInMol, NULL);
  	pthread_cond_init(&lastAdultCrossed, NULL);
  	pthread_cond_init(&showedUpInOahu, NULL);
  	pthread_cond_init(&finishedTransporting, NULL);
  	pthread_cond_init(&transportingChildren, NULL);
 
}