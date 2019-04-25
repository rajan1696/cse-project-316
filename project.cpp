    
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
	
	int total_passengers_boarded = 0;
	int max_free_seats_per_train;
	printf("Enter available free seats in the train: ");
	scanf("%d",&max_free_seats_per_train);
	int pass = 0;
	while (passengers_left > 0) {
		int free_seats = max_free_seats_per_train;
		
		printf("Train entering station with %d free seats\n", free_seats);
		load_train_returned = 0;
		struct load_train_args args = { &station, free_seats };
		pthread_t b;
		int cr = pthread_create(&b, NULL, train_thread, &args);
		if (cr != 0) {
			perror("pthread_create");
			exit(1);
		}

		int threads_to_take = MIN(passengers_left, free_seats);
		int threads_taken = 0;
		while (threads_taken< threads_to_take) {
			if (load_train_returned) {
				printf("Error: station_load_train returned early!\n");
			
				exit(1);
			}
			if (threads_completed > 0) {
				if ((pass% 2) == 0)
					usleep(2);
				threads_taken++;
				station_on_board(&station);
				__sync_sub_and_fetch(&threads_completed, 1);
			}
		}
		
		for (i = 0; i < 1000; i++) {
			if (i > 100 && load_train_returned)
				break;
			usleep(1000);
		}

		if (!load_train_returned) {
			printf("Error: station_load_train failed to return\n");
			exit(1);
		}

	while (threads_completed > 0) {
			threads_taken++;
			__sync_sub_and_fetch(&threads_completed, 1);
		}

		passengers_left -= threads_taken;
		total_passengers_boarded += threads_taken;
		printf("Train is going to depart from station with %d new passenger(s)\n",
			threads_to_take, threads_taken,
			(threads_to_take != threads_taken) ? " *****" : "");

		if (threads_to_take!= threads_taken) {
			printf("Error: Too many passengers on this train!\n");
			exit(1);
		}

		pass++;
	}

	if (total_passengers_boarded == total_passengers) {
		printf("Passengers seated in seats and train is ready to leave\n");
		
		
		return 0;
	} else {
		printf("Error: expected %d total boarded passengers, but got %d!\n");
		return 1;
	}
}
