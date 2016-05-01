/******************************************************************************
* FILE: condition.c
* AUTHORS: Quang Tran and Cody Bohlman
* DATE: April 27th 2016
******************************************************************************/



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
pthread_cond_t waitingForSecondChild;
pthread_cond_t rowing;

pthread_mutex_t loadBoat;
pthread_mutex_t boatMutex;
pthread_mutex_t numchildrenInBoatLock;
pthread_mutex_t inOahuChildLock;
pthread_mutex_t inOahuAdultLock;
pthread_mutex_t finished;


int childrenInOahu = 0;
int adultsInOahu = 0;

int numchildrenInBoat = 0;
bool boatIsInOahu = true;

sem_t* startInOahu;


int main(int args, char *argv[]){
	initSynch();
	printf("LET'S TRANSPORT PEOPLE FROM OAHU TO MOLOKAI!!!!\n\n\n");
	fflush(stdout);
	const int numchildren = atoi(argv[1]);
	const int numadults = atoi(argv[2]);
	pthread_t childthreads[numchildren];
	pthread_t adultthreads[numadults];
	
	startInOahu = sem_open("num_child", O_CREAT|O_EXCL, 0466, 0);
  	while (startInOahu==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore startInOahu already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("num_child");
	      startInOahu = sem_open("num_child", O_CREAT|O_EXCL, 0466, 0);
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
		err = sem_wait(startInOahu);
	}

	for (int i = 0; i < numadults; i++){
		pthread_create(&adultthreads[i], NULL, adult, NULL);
		err = sem_wait(startInOahu);
	}

  	pthread_cond_broadcast(&showedUpInOahu);

  	pthread_cond_wait(&finishedTransporting, &finished);
  	sem_close(startInOahu);
  	sem_unlink("num_child");
  	exit(1);

}

void* child(void* args){
	printf("Child arrived to OAHU\n");
	fflush(stdout);
	sem_post(startInOahu);	
	bool inOahu = true;

	pthread_mutex_lock(&inOahuChildLock);
	childrenInOahu++;
	pthread_cond_wait(&showedUpInOahu, &inOahuChildLock);
	pthread_mutex_unlock(&inOahuChildLock);
	while (true){
		if (inOahu) {
			pthread_mutex_lock(&boatMutex);
			while (!boatIsInOahu){
				pthread_cond_wait(&boatInOahu, &boatMutex);
			}
			pthread_mutex_lock(&loadBoat);

			pthread_mutex_lock(&inOahuChildLock);
			pthread_mutex_lock(&inOahuAdultLock);


			// Terminal case
			if (childrenInOahu == 1 && adultsInOahu == 0){
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_mutex_unlock(&inOahuAdultLock);
				printf("Last child boarded the boat in Oahu\n");
				fflush(stdout);
				printf("Last child rowing the boat\n");
				fflush(stdout);
				pthread_mutex_lock(&inOahuChildLock);
				childrenInOahu--;
				pthread_mutex_unlock(&inOahuChildLock);
				printf("Last child arrived to Mol\n");
				fflush(stdout);
				pthread_cond_signal(&finishedTransporting);
				pthread_exit(NULL);
			}

			// Terminal case
			else if(childrenInOahu == 2 && adultsInOahu == 0){
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_mutex_unlock(&inOahuAdultLock);
				if (numchildrenInBoat == 0){
					numchildrenInBoat++;
					pthread_mutex_unlock(&numchildrenInBoatLock);
					printf("First child boarded the boat in Oahu\n");
					fflush(stdout);
					pthread_mutex_unlock(&loadBoat);
					pthread_cond_wait(&waitingForSecondChild, &boatMutex);
					printf("First child rowing the boat\n");
					fflush(stdout);
					pthread_cond_signal(&rowing);
					pthread_mutex_lock(&inOahuChildLock);
					childrenInOahu--;
					pthread_mutex_unlock(&inOahuChildLock);
					pthread_cond_wait(&rowing, &boatMutex);
					printf("First child arrived to Mol\n");
					fflush(stdout);
					pthread_mutex_unlock(&boatMutex);
					pthread_cond_signal(&rowing);
					boatIsInOahu = false;
					
					inOahu = false;


				} else{
					numchildrenInBoat = 0;
					pthread_mutex_unlock(&numchildrenInBoatLock);
					printf("Second child boarded the boat in Oahu\n");
					fflush(stdout);
					pthread_cond_signal(&waitingForSecondChild);
					pthread_cond_wait(&rowing, &boatMutex);
					printf("Second child rowing the boat\n");
					fflush(stdout);
					pthread_cond_signal(&rowing);
					pthread_mutex_lock(&inOahuChildLock);
					childrenInOahu--;
					pthread_mutex_unlock(&inOahuChildLock);
					pthread_cond_wait(&rowing, &boatMutex);
					printf("Second child arrived to Mol\n");
					fflush(stdout);
					pthread_cond_signal(&finishedTransporting);
					inOahu = false;

				}
				pthread_exit(NULL);
			}

			// Adults start crossing
			else if(childrenInOahu == 1 && adultsInOahu != 0){
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_mutex_unlock(&inOahuAdultLock);
				pthread_mutex_unlock(&boatMutex);
				pthread_mutex_unlock(&loadBoat);
				pthread_cond_signal(&transportingChildren);
			}

			// Children crossing
			else{
				pthread_mutex_unlock(&inOahuChildLock);
				pthread_mutex_unlock(&inOahuAdultLock);
				pthread_mutex_lock(&numchildrenInBoatLock);
				if (numchildrenInBoat == 0){
					numchildrenInBoat++;
					pthread_mutex_unlock(&numchildrenInBoatLock);
					printf("First child boarded the boat in Oahu\n");
					fflush(stdout);

					pthread_mutex_unlock(&loadBoat);

					pthread_cond_wait(&waitingForSecondChild, &boatMutex);
					printf("First child rowing the boat\n");
					fflush(stdout);
					pthread_mutex_lock(&inOahuChildLock);
					childrenInOahu--;
					pthread_mutex_unlock(&inOahuChildLock);
					printf("First child arrived to Mol\n");
					fflush(stdout);
					pthread_mutex_unlock(&boatMutex);
					inOahu = false;

				} else{
					boatIsInOahu = false;
					printf("Second child boarded the boat in Oahu\n");
					fflush(stdout);
					pthread_mutex_unlock(&boatMutex);
					pthread_cond_signal(&waitingForSecondChild);
					pthread_mutex_unlock(&loadBoat);
					printf("Second child rowing the boat\n");
					fflush(stdout);

					pthread_mutex_lock(&inOahuChildLock);
					childrenInOahu--;
					pthread_mutex_unlock(&inOahuChildLock);
					printf("Second child arrived to Mol\n");
					fflush(stdout);
					numchildrenInBoat = 0;
					pthread_mutex_unlock(&numchildrenInBoatLock);
					inOahu = false;

				}
			}
		}

		// Child bringing the boat back from Molokai
		else {
			pthread_mutex_lock(&boatMutex);
			while (boatIsInOahu){
				pthread_cond_wait(&boatInMol, &boatMutex);
			}
			pthread_mutex_lock(&loadBoat);
			pthread_mutex_lock(&inOahuChildLock);
			
			printf("A child is rowing back from Mol to Oahu\n");
			fflush(stdout);
			childrenInOahu++;
			printf("A child arrived back to Oahu\n");
			fflush(stdout);
			inOahu = true;
			pthread_mutex_unlock(&inOahuChildLock);
			
			fflush(stdout);
			boatIsInOahu = true;
			pthread_cond_broadcast(&boatInOahu);
			
			pthread_mutex_unlock(&loadBoat);
			pthread_mutex_unlock(&boatMutex);
		}
	}
}

//Adults simply cross the river when they can
void* adult(void* args){
	printf("Adult arrived to OAHU\n");
	fflush(stdout);
	sem_post(startInOahu);
	pthread_mutex_lock(&inOahuAdultLock);
	pthread_cond_wait(&showedUpInOahu, &inOahuAdultLock);
	pthread_mutex_unlock(&inOahuAdultLock);
	bool inOahu = true;
	pthread_mutex_lock(&inOahuAdultLock);
	adultsInOahu++;
	pthread_mutex_unlock(&inOahuAdultLock);


	pthread_mutex_lock(&boatMutex);
	pthread_mutex_lock(&inOahuChildLock);
	while (childrenInOahu > 1) {
		pthread_mutex_unlock(&inOahuChildLock);
		pthread_cond_wait(&transportingChildren, &boatMutex);
	}
	pthread_mutex_unlock(&inOahuChildLock);

	while (!boatIsInOahu){
		pthread_cond_wait(&boatInOahu, &boatMutex);
	}
	
	
	printf("Adult started to cross\n");
	fflush(stdout);

	printf("Adult arrived in MOL\n");
	fflush(stdout);
	boatIsInOahu = false;
	inOahu = false;
	adultsInOahu--;
	pthread_cond_signal(&boatInMol);
	pthread_mutex_unlock(&boatMutex);
	return (void*) 0;
}

void initSynch() {
	pthread_mutex_init(&loadBoat, NULL);
	pthread_mutex_init(&boatMutex, NULL);
	pthread_mutex_init(&inOahuChildLock, NULL);
	pthread_mutex_init(&inOahuAdultLock, NULL);
	pthread_mutex_init(&numchildrenInBoatLock, NULL);
	pthread_mutex_init(&finished, NULL);

  	pthread_cond_init(&boatInOahu, NULL);
  	pthread_cond_init(&boatInMol, NULL);
  	pthread_cond_init(&showedUpInOahu, NULL);
  	pthread_cond_init(&finishedTransporting, NULL);
  	pthread_cond_init(&transportingChildren, NULL);
  	pthread_cond_init(&waitingForSecondChild, NULL);
  	pthread_cond_init(&rowing, NULL);
 
}