#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>


void* child(void*);
void* adult(void*);
void initSynch(void);

pthread_mutex_t loadBoat;
pthread_cond_t boatInOahu;
pthread_cond_t boatInMol;
pthread_cond_t lastAdultCrossed;
pthread_cond_t showedUpInOahu;
pthread_cond_t finishedTransporting;
pthread_cond_t transportingChildren;

pthread_mutex_t numChildrenInBoatLock;

pthread_mutex_t inOahuChildLock;
pthread_mutex_t inMolChildLock;
pthread_mutex_t inOahuAdultLock;
pthread_mutex_t inMolAdultLock;

int childrenInOahu = 0;
int adultsInOahu = 0;
int childrenInMol = 0;
int adultsInMol = 0;

int numChildrenInBoat = 0;

sem_t* numpeople;

int main(int args, char *argv[]){
	const int numchildren = atoi(argv[1]);
	const int numadults = atoi(argv[2]);
	pthread_t childthreads[numchildren];
	pthread_t adultthreads[numadults];

	numpeople = sem_open("num_people", O_CREAT|O_EXCL, 0466, 0);
  	while (numpeople==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore numpeople already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("num_people");
	      numpeople = sem_open("num_people", O_CREAT|O_EXCL, 0466, 10);
	    }
	    else {
	      printf("semaphore could not be opened, error # %d\n", errno);
	      fflush(stdout);
	      exit(1);
	    }
  	}

  	

	initSynch();
	
	int err;
	for (int i = 0; i < numchildren; i++){
		pthread_create(&childthreads[i], NULL, child, NULL);
		
		err = sem_wait(numpeople);
	}

	for (int i = 0; i < numadults; i++){
		pthread_create(&adultthreads[i], NULL, adult, NULL);
		printf("Adult stuff\n");
		fflush(stdout);
		err = sem_wait(numpeople);
	}
  	pthread_cond_broadcast(&showedUpInOahu);
  	printf("Done\n");
	fflush(stdout);

  	


  	pthread_cond_wait(&finishedTransporting, NULL);
  	sem_close(numpeople);
  	sem_unlink("num_people");
  	exit(1);

}

void* child(void* args){
	printf("Child %s arrived to OAHU\n", (char*) args);
	fflush(stdout);
	sem_post(numpeople);	
	bool inOahu = true;
	pthread_mutex_lock(&inOahuChildLock);
	childrenInOahu++;
	pthread_mutex_unlock(&inOahuChildLock);
	pthread_cond_wait(&showedUpInOahu, NULL);
	printf("Everyone has arrived");
	fflush(stdout);
	
	while (1){
		if (inOahu){
			pthread_mutex_lock(&inOahuChildLock);
			if (childrenInOahu > 1){
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_mutex_lock(&loadBoat);
				pthread_cond_wait(&boatInOahu, &loadBoat);
				pthread_mutex_lock(&numChildrenInBoatLock);
				numChildrenInBoat++;
				if (numChildrenInBoat == 1){
					printf("Child %s boarded boat in OAHU\n", (char*) args);
					fflush(stdout);
					pthread_mutex_unlock(&loadBoat);
				}
				else{
					printf("Child %s boarded boat in OAHU\n", (char*) args);
					fflush(stdout);
					pthread_mutex_lock(&inOahuChildLock);
					pthread_mutex_lock(&inMolChildLock);
					childrenInOahu -= 2;
					childrenInMol += 2;
					numChildrenInBoat = 0;

				}
				inOahu = false;
				pthread_mutex_unlock(&numChildrenInBoatLock);
				printf("Child %s arrived in MOL\n", (char*) args);
				fflush(stdout);
				pthread_cond_signal(&boatInMol);

			}
			else{
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_cond_broadcast(&transportingChildren);
				pthread_mutex_lock(&inOahuAdultLock);
				if (adultsInOahu == 0){
					pthread_mutex_unlock(&inOahuAdultLock);
					pthread_cond_wait(&boatInOahu, NULL);
					pthread_mutex_lock(&loadBoat);
					inOahu = false;
					childrenInMol++;
					childrenInOahu--;
					pthread_cond_signal(&boatInMol);
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
			printf("Child %s arrived in OAHU\n", (char*) args);
			fflush(stdout);
			pthread_cond_signal(&boatInOahu);
			pthread_mutex_unlock(&inOahuChildLock);
			pthread_mutex_unlock(&inMolChildLock);
		}
		
	}


}

void* adult(void* args){
	printf("Adult %s arrived to OAHU\n", (char*) args);
	fflush(stdout);
	sem_post(numpeople);
	pthread_cond_wait(&showedUpInOahu, NULL);
	printf("Everyone has arrived");
	fflush(stdout);
	bool inOahu = true;
	pthread_mutex_lock(&inOahuAdultLock);
	adultsInOahu++;
	pthread_mutex_unlock(&inOahuAdultLock);
	pthread_cond_wait(&transportingChildren, NULL);
	pthread_cond_wait(&boatInOahu, NULL);
	pthread_mutex_lock(&loadBoat);
	printf("Adult %s arrived in MOL\n", (char*) args);
	fflush(stdout);
	inOahu = false;
	adultsInOahu--;
	adultsInMol++;
	pthread_mutex_unlock(&loadBoat);
	pthread_cond_signal(&boatInMol);
	return (void*) 0;
}

void initSynch(){

	pthread_mutex_init(&loadBoat, NULL);
	pthread_mutex_init(&inOahuChildLock, NULL);
	pthread_mutex_init(&inMolChildLock, NULL);
	pthread_mutex_init(&inOahuAdultLock, NULL);
	pthread_mutex_init(&inMolAdultLock, NULL);
  	pthread_cond_init (&boatInOahu, NULL);
  	pthread_cond_init (&boatInMol, NULL);
  	pthread_cond_init (&lastAdultCrossed, NULL);
  	pthread_cond_init (&showedUpInOahu, NULL);
  	pthread_cond_init (&finishedTransporting, NULL);
  	pthread_cond_init (&transportingChildren, NULL);
  	
  	



}