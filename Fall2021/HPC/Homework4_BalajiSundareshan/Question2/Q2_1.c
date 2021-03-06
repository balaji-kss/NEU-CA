#include <stdbool.h>
#include <mpi.h>  
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define MIN_NUM 1
#define MAX_NUM 1000

int * data;
int * local_data;
int * local_data_lens;
int * data_stride;
int * displs;
int num_procs, global_data_len;
int num_classes;
int proc_rank, name_len;
char processor_name[MPI_MAX_PROCESSOR_NAME];

int * classes;
int * local_classes;
float * min_range_cls;
float * max_range_cls;
int * local_data_cls_flag;
int * global_data_cls_flag;

void gen_data(int global_data_len)
{

    data = (int *)calloc(global_data_len, sizeof(int));

    int i;
    for(i=0; i<global_data_len; i++){
        data[i] = rand() % MAX_NUM + MIN_NUM;
    }

}

void set_classes(int num_classes){
    
    int i;
    
    float range = MAX_NUM - MIN_NUM;
    float interval = (float) range / (float) num_classes;
    interval = ceil(interval);

    for(i=0; i<num_classes; i++){

        min_range_cls[i] = i*interval + MIN_NUM;

        if(i == num_classes-1){
            max_range_cls[i] = MAX_NUM + 1;
        }else{
            max_range_cls[i] = (i+1)*interval + MIN_NUM;
        }
        
        local_classes[i] = 0;
    }

}

void print_data(int global_data_len, int num_classes){

    int i;

    printf("Input data: \n");
    for(i=0; i<global_data_len; i++){
        printf("%d ",data[i]);
    }
    printf("\n");

    printf("Class range: \n");
    for(i=0; i<num_classes; i++){
        printf("Class %d Min: %f, Max: %f\n", i, min_range_cls[i], max_range_cls[i]);
    }
}

void print_hist(int num_classes, int * hist_classes){

    int i;
    int total_vals = 0;
    for(i=0; i<num_classes; i++){
        printf("Number of values in Class %d: %d\n", i, hist_classes[i]);
        total_vals += hist_classes[i];
    }
    printf("total values in histogram: %d\n", total_vals);
}

void group_data_bins(int local_data_len, int num_classes, int proc_rank){

    int i, class_;
    for(i = 0; i < local_data_len; i++) 
    {
        class_ = find_bin(local_data[i], num_classes);  
        assert(("class index should not be negative", class_>=0));
        local_classes[class_] += 1;
        local_data_cls_flag[displs[proc_rank] + i] = class_;
    }

}

int find_bin(int local_data_val, int num_classes){

    int i;
    for(i = 0; i < num_classes; i++) 
    {
        if(local_data_val>=min_range_cls[i] && local_data_val<max_range_cls[i])
            return  i;
    }

    return -1;
}

void dist_data(int global_data_len, int num_classes, int num_proc){

    local_data_lens = (int *)calloc(num_proc, sizeof(int));
    int data_bin_len = global_data_len / num_classes;

    int i;
    for(i=0; i<num_classes; i++){
        local_data_lens[i%num_proc] += data_bin_len;
    }

    displs = (int *)calloc(num_proc, sizeof(int));
    
    displs[i] = 0;
    for (i=1; i<num_proc; i++) { 
        displs[i] = displs[i-1] + local_data_lens[i-1]; 
    } 
}

void print_scatter_params(int num_proc){

    int i, j;
    printf("Data send count for each proc:\n");
    for(i=0; i<num_proc; i++){
        printf("proc %d: %d\n", i, local_data_lens[i]);
    }
    
    printf("Displacement for each proc:\n");
    for(i=0; i<num_proc; i++){
        printf("proc %d: %d\n", i, displs[i]);
    }
}

void print_data_buffer(int proc_rank){

    int i;
    for (i = 0; i < local_data_lens[proc_rank]; i++) {
        printf("%d, ", local_data[i]);
    }
    printf("\n");
}

void print_results()
{
    int i, j;
    for(i=0;i<num_classes;i++){
        printf("Number of values in Class %d: %d Values are: ", i, classes[i]);
        for(j=0;j<global_data_len;j++){
            if(global_data_cls_flag[j]==i){
                printf("%d ",data[j]);
            }
        }
        printf("\n");  
    }
}

int main(int argc, char *argv[])
{

    assert(("./Q2_1 <number of values> <number of classes>", argc == 3));

    global_data_len = atoi(argv[1]);
    num_classes = atoi(argv[2]);

    assert(("Num_values should be evenly divisible by num_classes", global_data_len % num_classes == 0));

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);
    MPI_Get_processor_name(processor_name, &name_len); 

    if(proc_rank==0){
    printf("Number of values: %d\n", global_data_len);
    printf("Number of classes: %d\n", num_classes);}

    double start_time, end_time, time_elapsed_procs, total_time_elapsed;
    local_classes = (int *)calloc(num_classes, sizeof(int));
    min_range_cls = (float *)calloc(num_classes, sizeof(float));
    max_range_cls = (float *)calloc(num_classes, sizeof(float));

    local_data_cls_flag = (int *)calloc(global_data_len, sizeof(int));
    global_data_cls_flag = (int *)calloc(global_data_len, sizeof(int));

    // Data initialize
    gen_data(global_data_len);   
    set_classes(num_classes);

    MPI_Bcast(&num_classes, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&global_data_len, 1, MPI_INT, 0, MPI_COMM_WORLD);

    dist_data(global_data_len, num_classes, num_procs);

    if(proc_rank == 0){
        print_data(global_data_len, num_classes);
        print_scatter_params(num_procs);
    }

    local_data = (int *)calloc(local_data_lens[proc_rank], sizeof(int));
    classes = (int *)calloc(num_classes, sizeof(int));

    MPI_Scatterv(data, local_data_lens, displs, MPI_INT, local_data, local_data_lens[proc_rank], MPI_INT, 0, MPI_COMM_WORLD);

    printf("processor rank: %d len: %d\n", proc_rank, local_data_lens[proc_rank]);
    //print_data_buffer(proc_rank);
    // printf("Histogram for process %d on %s out of %d:\n", proc_rank, processor_name, num_procs);
    //print_hist(num_classes, local_classes);

    start_time = MPI_Wtime();
    group_data_bins(local_data_lens[proc_rank], num_classes, proc_rank);
    end_time = MPI_Wtime();
    time_elapsed_procs = end_time - start_time;

    // reduction in time
    MPI_Reduce(&time_elapsed_procs, &total_time_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD); 

    // reduction in local histogram
    MPI_Reduce(local_classes, classes, num_classes, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // reduction in data values
    MPI_Reduce(local_data_cls_flag, global_data_cls_flag, global_data_len, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Finalize();

    if(proc_rank==0){
        printf("Final Histogram:\n");
        print_hist(num_classes, classes);
        print_results();
        printf("Elapsed time: %f seconds \n", total_time_elapsed);
    }

    return 0;

}