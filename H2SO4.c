#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "H2SO4.h"

sem_t* hydro_sem;
sem_t* oxygen_sem;

void* oxygen(void* arg){

  	printf("oxygen produced\n");
  	fflush(stdout);

	// post (call up) on hydrogen semaphore to signal that a hydrogen atom
	// has been produced
	sem_post(oxygen_sem);
	return (void*) 0;
}

void* hydrogen(void* arg){
  	printf("hydrogen produced\n");
  	fflush(stdout);

	// post (call up) on hydrogen semaphore to signal that a hydrogen atom
	// has been produced
	sem_post(hydro_sem);

	// hydrogen exits
	//printf("hydrogen leaving\n");
	//fflush(stdout);
  
  return (void*) 0;
}
void* sulfur(void* arg){
  	printf("sulfur produced\n");
  	fflush(stdout);

  	// oxygen waits (calls down) twice on the hydrogen semaphore
  	// meaning it cannot continue until at least 2 hydrogen atoms
  	// have been produced
  	int err = sem_wait(hydro_sem);
  	int err2 = sem_wait(hydro_sem);
  	int err3 = sem_wait(oxygen_sem);
  	int err4 = sem_wait(oxygen_sem);
  	int err5 = sem_wait(oxygen_sem);
  	int err6 = sem_wait(oxygen_sem);

  	if (err==-1 || err2==-1 || err3==-1 || err4==-1 || err5==-1 || err6==-1) printf("error on oxygen wait for hydro_sem, error # %d\n", errno);
  
  	// produce a sulfuric acid
  	printf("made H2SO4\n");
  	fflush(stdout);
	return (void*) 0;
}
void openSems(){
	hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
	oxygen_sem = sem_open("oxysmphr", O_CREAT|O_EXCL, 0466, 0);
	while (hydro_sem==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore hydrosmphr already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("hydrosmphr");
	      hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
	    }
	    else {
	      printf("semaphore could not be opened, error # %d\n", errno);
	      fflush(stdout);
	      exit(1);
	    }
  	}
  	while (oxygen_sem==SEM_FAILED) {
	    if (errno == EEXIST) {
	      printf("semaphore oxysmphr already exists, unlinking and reopening\n");
	      fflush(stdout);
	      sem_unlink("oxysmphr");
	      oxygen_sem = sem_open("oxysmphr", O_CREAT|O_EXCL, 0466, 0);
	    }
	    else {
	      printf("semaphore could not be opened, error # %d\n", errno);
	      fflush(stdout);
	      exit(1);
	    }
  	}

}
void closeSems(){
	sem_close(hydro_sem);
  	sem_unlink("hydrosmphr");
  	sem_close(oxygen_sem);
  	sem_unlink("oxysmphr");
}
