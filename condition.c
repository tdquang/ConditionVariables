#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>



pthread_mutex_t loadBoat;


pthread_cond_t boatFull;
pthread_cond_t halfFull;
pthread_cond_t boatEmpty;




pthread_cond_t inOahuWithRoom;
pthread_cond_t inMolWithRoom;
pthread_cond_t showedUpInOahu;
pthread_cond_t finishedTransporting;
pthread_mutex_t peopleInOahu;

pthread_mutex_t inOahuChildLock;
pthread_mutex_t inMolChildLock;
pthread_mutex_t inOahuAdultLock;
pthread_mutex_t inMolAdultLock;

int childrenInOahu = 0;
int adultsInOahu = 0;
int childrenInMol = 0;
int adultInMol = 0;


sem_t* numpeople;

int main(int args, char *argv[]){
	const int numchildren = atoi(argv[1]);
	const int numadults = atoi(argv[2]);
	pthread_t childthreads[numchildren];
	pthread_t adultthreads[numadults];
	initSynch();
	

	for (int i = 0; i < numchildren; i++){
		pthread_create(&childthreads[i], NULL, child, NULL);
		sem_wait(numpeople);

	}

	for (int i = 0; i < numadults; i++){
		pthread_create(&adultthreads[i], NULL, adult, NULL);
		sem_wait(numpeople);
	}
  	
  	

  	pthread_cond_broadcast(&showedUpInOahu, NULL);




  	pthread_cond_wait(&finishedTransporting, NULL);
  	exit(1);

}

void* child(void* args){
	printf("Child %s arrived to OAHU\n", (char*) args);
	fflush(stdout);
	sem_post(numpeople);
	bool inOahu = 1;
	pthread_mutex_lock(&inOahuChildLock);
	childrenInOahu++;
	pthread_mutex_unlock(&inOahuChildLock);
	pthread_cond_wait(&showedUpInOahu, NULL);
	
	while (1){
		if (inOahu){
			pthread_mutex_lock(&inOahuChildLock);
			if (childrenInOahu > 1){
				pthread_cond_wait(&inOahuWithRoom, &inOahuChildLock);
				

			}
		}
		pthread_mutex_lock(&inOahuChildLock);
		
		if (childrenInOahu != 0 && adultsInOahu != 0){
			pthread_mutex_unlock(&inOahuChildLock);
			pthread_mutex_unlock(&inOahuAdultLock);

		}
		else{
			pthread_mutex_unlock(&inOahuChildLock);
			pthread_mutex_unlock(&inOahuAdultLock);
			pthread_exit(NULL);

		}
	}


}

void* adult(void* args){
	printf("Adult %s arrived to OAHU\n", (char*) args);
	fflush(stdout);
	sem_post(numpeople);
	pthread_cond_wait(&showedUpInOahu, NULL);
	bool inOahu = 1;
	pthread_mutex_lock(&inOahuAdultLock);
	adultsInOahu++;
	pthread_mutex_unlock(&inOahuAdultLock);
	pthread_cond_wait(&transportingChildren, NULL);
	
}

void initSynch(){
	pthread_mutex_init(&loadBoat, NULL);
	pthread_mutex_init(&inOahuChildLock, NULL);
	pthread_mutex_init(&inMolChildLock, NULL);
	pthread_mutex_init(&inOahuAdultLock, NULL);
	pthread_mutex_init(&inMolAdultLock, NULL);
  	pthread_cond_init (&boatFull, NULL);
  	pthread_cond_init (&showedUpInOahu, NULL);
  	pthread_cond_init (&finishedTransporting, NULL);
  	pthread_cond_init (&transportingChildren, NULL);
  	numpeople = sem_open("numpeople", O_CREAT|O_EXCL, 0466, numadults + numchildren);
  	while (numpeople==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore numpeople already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("numpeople");
	      numpeople = sem_open("numpeople", O_CREAT|O_EXCL, 0466, 0);
	    }
	    else {
	      printf("semaphore could not be opened, error # %d\n", errno);
	      fflush(stdout);
	      exit(1);
	    }
  	}
  	



}