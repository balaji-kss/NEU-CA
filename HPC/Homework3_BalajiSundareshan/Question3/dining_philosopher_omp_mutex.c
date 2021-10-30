#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

# define END_TIME 10000
pthread_mutex_t print_mutex;

typedef struct {

    int position;
    int count;
    int min_dur;
    int max_dur;
    int num_times_think;
    int num_times_eat;
    int total_think_duration;
    int total_eat_duration;
    pthread_mutex_t *left_fork;
    pthread_mutex_t *right_fork;

} philosopher_t;

void create_forks(pthread_mutex_t *forks, int num_forks);
void start_activity_philosopher(philosopher_t * philosopher);
void think(philosopher_t *philosopher);
void take_forks(philosopher_t *philosopher);
void eat(philosopher_t *philosopher);
void place_forks(philosopher_t *philosopher);
int is_last_philosopher(philosopher_t *philosopher);
float get_random_number(int min, int max);
int get_microseconds(int milliseconds);
void print_philosopher_stats(philosopher_t * philosopher);

int main(int argc, char *argv[])
{
    srand(time(NULL));
    assert(("./dining_philosopher_omp <num_philosopher> <min_dur_microsec> <max_dur_microsec>", argc == 4));
    
    int num_philosophers = atoi(argv[1]);
    int min_dur = atoi(argv[2]);
    int max_dur = atoi(argv[3]);
    int num_forks = num_philosophers;

    assert(("max_dur should be >= min_dur", max_dur>=min_dur));

    printf("...................................\n");
    printf("Number of Philosophers: %d\n", num_philosophers);
    printf("Number of Forks: %d\n", num_forks);
    printf("Expected Running Time: %d secs \n", END_TIME/1000);
    printf("Minimum Duration to Think/Eat: %d msecs \n", min_dur);
    printf("Maximum Duration to Think/Eat: %d msecs \n", max_dur);
    printf("...................................\n\n");

    pthread_mutex_t forks[num_forks];
    create_forks(forks, num_forks);

    #pragma omp parallel num_threads(num_philosophers)
    {
        int thread_num = omp_get_thread_num();
		printf("Thread ID: %d started!\n",thread_num);

        philosopher_t *philosopher = malloc(sizeof(philosopher_t));
        philosopher->position = thread_num;
        philosopher->count = num_philosophers;
        philosopher->num_times_think = 0;
        philosopher->num_times_eat = 0;
        philosopher->total_think_duration = 0;
        philosopher->total_eat_duration = 0;
        philosopher->min_dur = min_dur;
        philosopher->max_dur = max_dur;
        philosopher->left_fork = &forks[thread_num];
        philosopher->right_fork = &forks[(thread_num + 1) % num_philosophers];
        
        start_activity_philosopher(philosopher);

    }

    return 0;
}

void start_activity_philosopher(philosopher_t * philosopher)
{

  time_t start_t, end_t;
  time(&start_t);

  while(1)
  {
    think(philosopher);
    take_forks(philosopher);
    eat(philosopher);
    place_forks(philosopher);
    time(&end_t);
    
    if(difftime(end_t, start_t)*1000>END_TIME){
        sleep(1);
        print_philosopher_stats(philosopher);
        break;
    }
  }
}

void create_forks(pthread_mutex_t *forks, int num_forks)
{ 
    int i;
    for(i = 0; i < num_forks; i++) {
       pthread_mutex_init(&forks[i], NULL);
    }
}

void think(philosopher_t *philosopher)
{

    int think_duration; 
    think_duration = get_random_number(philosopher->min_dur, philosopher->max_dur);
    philosopher->total_think_duration += think_duration;
    printf("Philosopher %d started thinking for %d ms\n", philosopher->position, think_duration);

    // sleep
    float microsec = get_microseconds(think_duration);
    usleep(microsec);

    philosopher->num_times_think += 1;

    printf("Philosopher %d stopped thinking\n", philosopher->position);

}

void take_forks(philosopher_t *philosopher)
{ 
  if (is_last_philosopher(philosopher))
  {
      pthread_mutex_lock(philosopher->right_fork);
      pthread_mutex_lock(philosopher->left_fork);
  }
  else
  {
      pthread_mutex_lock(philosopher->left_fork);
      pthread_mutex_lock(philosopher->right_fork);
  }
}

void eat(philosopher_t *philosopher)
{
    int eat_time;
    eat_time = get_random_number(philosopher->min_dur, philosopher->max_dur);
    philosopher->total_eat_duration += eat_time;

    printf("Philosopher %d started eating for %d ms\n", philosopher->position, eat_time);
    // sleep
    float microsec = get_microseconds(eat_time);
    usleep(microsec);

    philosopher->num_times_eat += 1;

    printf("Philosopher %d stopped eating\n", philosopher->position);

}

void place_forks(philosopher_t *philosopher)
{   
    pthread_mutex_unlock(philosopher->right_fork);
    pthread_mutex_unlock(philosopher->left_fork);
}

int is_last_philosopher(philosopher_t *philosopher)
{
    return philosopher->position == philosopher->count-1;
}

float get_random_number(int min, int max)
{

    if (min==max){
      return min;
    }

    float result = (rand() % (max + 1));
    if (result < min) result = min;
    return result;

}

int get_microseconds(int milliseconds){

    return milliseconds*1000;
}

void print_philosopher_stats(philosopher_t * philosopher){

  pthread_mutex_lock (&print_mutex);
  printf("--------------------------------------------\n");
  printf("Philosopher %d stats\n",philosopher->position);
  printf("Philosopher %d number of times thought: %d \n", philosopher->position, philosopher->num_times_think);
  printf("Philosopher %d number of plates eaten: %d \n", philosopher->position, philosopher->num_times_eat);
  printf("Philosopher %d total eat duration: %d ms\n", philosopher->position, philosopher->total_eat_duration);
  printf("Philosopher %d total think duration: %d ms\n", philosopher->position, philosopher->total_think_duration);
  printf("--------------------------------------------\n");
  pthread_mutex_unlock (&print_mutex);

}