# include <stdlib.h>
# include <pthread.h>
# include <iostream>
# include <vector>
# include <unistd.h>
# include <cstdlib>
# include <cstring>
# include <sched.h>
# include <chrono>

using namespace std;

typedef struct {
    int thread_id;
    int sched_policy;
    int sched_priority;
    float sched_time_slice;
} Arg;


pthread_barrier_t print_barrier;

void *print_thread(void *args)
{
    /* 1. Wait until all threads are ready */
    Arg *arg = (Arg *) args;
    int thread_id = arg -> thread_id;
    float time_slice = arg -> sched_time_slice;
    
    pthread_barrier_wait(&print_barrier);

    /* 2. Do the task */ 
    for (int i = 0; i < 3; i++) {
        printf("Thread %d is running\n", thread_id);
        
        time_t start = time(NULL);
        /* Busy for <time_wait> seconds */
        while (1) { 
            if ((time(NULL) - start) > time_slice)
                break;
        }

    }
    /* 3. Exit the function  */
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int num_threads, opt;
    float time_slice;
    vector<int> schedule_policy;
    vector<int> priority;

    /* 1. Parse program arguments */
    while ((opt = getopt(argc, argv, "n:t:s:p:")) != -1) {
        switch(opt) {
            case 'n':
                num_threads = atoi(optarg);
                break;
            
            case 't':
                time_slice = atof(optarg);
                break;
            
            case 's':
                for (char *token = strtok(optarg, ","); token != nullptr; token = strtok(nullptr, ",")) {
                    if (strcmp(token, "NORMAL") == 0) {
                        schedule_policy.push_back(SCHED_OTHER);
                    }
                    else if (strcmp(token, "FIFO") == 0){
                        schedule_policy.push_back(SCHED_FIFO);
                    }
                }
                break;
            case 'p':
                for (char *token = strtok(optarg, ","); token != nullptr; token = strtok(nullptr, ",")) {
                    priority.push_back(atoi(token));
                }
                break;
            default:
                cerr << "Usage: " << argv[0] << " -n <num_threads> -t <time_slice> -s <policies> -p <priorities>\n";
                return 1;
        }
    }
    
    /* 2. Create <num_threads> worker threads */
    pthread_t threads[num_threads];
    
    /* 5. Start all threads at once */
    pthread_barrier_init(&print_barrier, NULL, num_threads);

    Arg args[num_threads];
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        args[i].sched_policy = schedule_policy[i];
        args[i].sched_priority = priority[i];
        args[i].sched_time_slice = time_slice;
    }
    
    /* 3. Set CPU affinity */
    cpu_set_t cpu_set;
    // Initial CPU Set and dedicate one CPU to a particular thread
    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);
    
    for (int i = 0; i < num_threads; i++) {    
        struct sched_param param;
        param.sched_priority = args[i].sched_priority;

        /* 4. Set the attributes to each thread */
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu_set);
        pthread_attr_setschedpolicy(&attr, args[i].sched_policy);
        pthread_attr_setschedparam(&attr, &param);

        // Create thread and set its corresponding arg and attr
        pthread_create(&threads[i], &attr, print_thread, (void *) &args[i]);

        pthread_attr_destroy(&attr);
    }

    /* 6. Wait for all threads to finish  */ 
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&print_barrier);
}