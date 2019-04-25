    
#include <stdlib.h>
#include<stdio.h>
#include<conio.h>
#include <signal.h> 
#include <unistd.h>
#include <pthread.h>
#ifndef MIN
#define MIN(_x,_y) ((_x) < (_y)) ? (_x) : (_y)
#endif

int load_train_returned = 0;
int threads_completed = 0;
struct station {
  int waiting_passengers; // in station waiting passengers
  int seated_passengers; // in train passengers 
  pthread_mutex_t lock;
  pthread_cond_t train_arrived;
  pthread_cond_t passengers_seated;
  pthread_cond_t train_full;
};
struct load_train_args {
	struct station *station;
	int free_seats;
};

void station_init(struct station *station);
void station_load_train(struct station *station, int count);
void station_wait_for_train(struct station *station);
void station_on_board(struct station *station);

/**
  Initializes all the mutexes and condition-variables.
*/
void station_init(struct station *station)
{
  station->waiting_passengers = 0;
  station->seated_passengers = 0;
  pthread_mutex_init(&(station->lock), NULL);
  pthread_cond_init(&(station->train_arrived), NULL);
  pthread_cond_init(&(station->passengers_seated), NULL);
  pthread_cond_init(&(station->train_full), NULL);
}


void station_load_train(struct station *station, int count)
{
  // Enter critical region
  pthread_mutex_lock(&(station->lock));

  while ((station->waiting_passengers > 0) && (count > 0)){
    pthread_cond_signal(&(station->train_arrived));
  	count--;
  	pthread_cond_wait(&(station->passengers_seated), &(station->lock));
  }

  if (station->seated_passengers > 0)
  	pthread_cond_wait(&(station->train_full), &(station->lock));

  // Leave critical region
  pthread_mutex_unlock(&(station->lock));
}

void station_wait_for_train(struct station *station)
{
  pthread_mutex_lock(&(station->lock));

  station->waiting_passengers++;
  pthread_cond_wait(&(station->train_arrived), &(station->lock));
  station->waiting_passengers--; /* passenger got the seat */
  station->seated_passengers++;

  pthread_mutex_unlock(&(station->lock));

  pthread_cond_signal(&(station->passengers_seated));
}

void station_on_board(struct station *station)
{
  pthread_mutex_lock(&(station->lock));

  station->seated_passengers--;

  pthread_mutex_unlock(&(station->lock));

  if (station->seated_passengers == 0)
  	pthread_cond_broadcast(&(station->train_full));
}



void* passenger_thread(void *arg)
{
	struct station *station = (struct station*)arg;
	station_wait_for_train(station);
	__sync_add_and_fetch(&threads_completed, 1);
	return NULL;
}


void* train_thread(void *args)
{
	struct load_train_args *l = (struct load_train_args*)args;
	station_load_train(l->station, l->free_seats);
	load_train_returned = 1;
	return NULL;
}

int main()
{
     int total_passengers; 
    printf("************************************************************************************************************************\n");
    printf("************************************************************************************************************************\n");
    printf("||||||||||||||||||||||||||||||||||||||  /**INDIAN RAIL WELCOMES YOU **/|||||||||||||||||||||||||||||||||||||||||||||||||\n");
    printf("|||||||||||||||||||||||||||||||||||||| /**TRAVEL SAFE TRAVEL WITH COMFORT**/ |||||||||||||||||||||||||||||||||||||||||||\n");
    printf("************************************************************************************************************************\n");
    printf("************************************************************************************************************************\n");
    printf("\nEnter Total No Of Passengers Waiting At The Station: ");
	scanf("%d",&total_passengers);
//	printf("\n  Total No of passengers are : %d\n",total_passengers);
	struct station station;
	station_init(&station);
	int passengers_left = total_passengers;
	station_load_train(&station, 0);
	station_load_train(&station, 10);
	// Create a bunch of 'passengers', each in their own thread.
	int i;
	for (i = 0; i < total_passengers; i++) {
		pthread_t a;
		int cr = pthread_create(&a, NULL, passenger_thread, &station);
		if (cr != 0) {
			perror("pthread_create");
			exit(1);
		}
	}

