#define _GNU_SOURCE
#include <pthread.h>
#include "bank.h"
#include "string_parser.h"
#include "account.h"
#include <stdlib.h> // for malloc
#include <stdio.h> // i/o functions, file stuff
#include <string.h> // for strcmp
#include <math.h> // for rounding


account **ACC;
int NUM_OF_ACC;
int NUM_OF_TRANSACTION = 0;
int NUM_OF_UPDATES = 0;
int NUM_OF_WORKERS = 10;
long JOBS_PER_WORKER;
int THREAD_CREATED_COUNT = 0;
int BANK_FLAG = 0;
int WORKERS_FINISHED = 0;

pthread_mutex_t bank_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t transaction_count = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t transfer_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t worker_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t worker_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t bank_cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;// barrier sycnro object
pthread_barrier_t barrier12;// barrier sycnro object


// hell grind valgrind flag for  race conditions in part3
int main(int argc, char *argv[]) {
    // file initiaization
    char *line = NULL;
    size_t length = 0;

    // open input file
    FILE *file = fopen(argv[2], "r");
    if (file == NULL) {
        perror("couldn't open file");
        printf("file:%s\n", argv[2]);
        return EXIT_FAILURE;
    }
    // retrieve number of accounts
    command_line line_parsed;
    getline(&line, &length, file);
    line_parsed = str_filler(line, " ");
    NUM_OF_ACC = atoi(line_parsed.command_list[0]);
    free_command_line(&line_parsed);
    free(line);
    line = NULL;

    // create and initialize array of account class objects
    populate_acc(file);

    // calculate the number of jobs per worker
    JOBS_PER_WORKER = num_jobs_per_worker(file, line, length);

    // create array of num_of_workers x JOBS_PER_WORKER x 128
    // 128 is for the lines
    char ***workers_ops;
    assigning_workers_ops(file, &workers_ops, JOBS_PER_WORKER);

    pthread_t bank_thread;
    pthread_t worker_threads[NUM_OF_WORKERS];
    void* (*banker_func)(void*) = &update_balance;
    // void* (*worker_func)(void*) = &process_transaction;
    pthread_barrier_init(&barrier, NULL, NUM_OF_WORKERS);
    pthread_barrier_init(&barrier12, NULL, 12);

    // // halt until lock if free (it's free)
    // pthread_mutex_lock(&bank_lock); 
    // bank thread is locked till all workers are finished
    pthread_create(&bank_thread, NULL, banker_func, NULL);


    // // create 10 worker threads
    for (int i = 0; i < NUM_OF_WORKERS; i++) {
        if (pthread_create(&worker_threads[i], NULL, process_transaction, (void *)(workers_ops[i])) != 0) {
            perror("worker thread failed to be created");
            return EXIT_FAILURE;
        }
    }
    // wait until worker threads have completed transactions
    pthread_barrier_wait(&barrier12);

    // main thread waiting for worker threads to be completed
    for(int i = 0; i < NUM_OF_WORKERS; i++){
        pthread_join(worker_threads[i], NULL);
    }
    WORKERS_FINISHED = 1;
    pthread_cond_signal(&bank_cond);
    pthread_mutex_unlock(&bank_lock);
    // allow bank thread to run and make main thread wait till its done
    // pthread_mutex_unlock(&bank_lock);
    pthread_join(bank_thread, NULL);

    fclose(file);
    file = fopen("output/output.txt", "a");
    for (int i = 0; i < NUM_OF_ACC; i++) {
        fprintf(file, "%d balance:\t%.2f\n\n", i, ACC[i]->balance);
    }
    fclose(file);

    printf("Number of rewards applied: %d\n", NUM_OF_UPDATES);
    // freeing and destroying

    // free mutexes
    // freeing account info array
    for (int i = 0; i < NUM_OF_ACC; i++) {
        pthread_mutex_destroy(&(ACC[i]->ac_lock));
        free(ACC[i]);
    }
    free(ACC);
    pthread_mutex_destroy(&transaction_count);
    pthread_mutex_destroy(&bank_lock);

    // freeing worker job distribution array
    for (int i = 0; i < NUM_OF_WORKERS; i++) {
        for (int j = 0; j < JOBS_PER_WORKER; j++) {
            free(workers_ops[i][j]);
        }
        free(workers_ops[i]);
    }
    free(workers_ops);
}

void assigning_workers_ops(FILE *file, char ****workers_ops, int JOBS_PER_WORKER) {
    // allocation of 3-d array
    *workers_ops = (char ***)malloc(sizeof(char **) * NUM_OF_WORKERS);
    for (int i = 0; i < NUM_OF_WORKERS; i++) {
        (*workers_ops)[i] = (char **)malloc(sizeof(char*) * JOBS_PER_WORKER);
        for (int j = 0; j < JOBS_PER_WORKER; j++) {
            (*workers_ops)[i][j] = (char *)malloc(sizeof(char) * 128);
        }
    }

    // loop through file evenly distributing jobs to each worker
        // file initiaization
    char *line = NULL;
    size_t length = 0;
    long saved_pos = ftell(file);
    if (saved_pos == -1) { // possible error
        perror("ftell failed to save file pos");
        // return EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    // distrbute operations among worker evenly
    int num_of_ops = 0; // number of total operations
    int worker_op_i = 0; // count how many ops each thread holds
    while (getline(&line, &length, file) != -1) {
        strcpy((*workers_ops)[num_of_ops % NUM_OF_WORKERS][worker_op_i], line);
        num_of_ops++;
        if ((num_of_ops % NUM_OF_WORKERS) == 0) {
            worker_op_i++;
        }
        free(line);
        line = NULL;
    }
    free(line);
    // reset file pointer to first op
    if (fseek(file, saved_pos, SEEK_SET) != 0) {
        perror("fseek failed");
        // return EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
}

long num_jobs_per_worker(FILE *file, char *line, size_t length) {
    // how many total operations do we have to do?
    // save file pointer position that's at first op
    long saved_pos = ftell(file);
    if (saved_pos == -1) { // possible error
        perror("ftell failed to save file pos");
        exit(EXIT_FAILURE);
    }
    // count number of ops
    long num_of_ops = 0;
    while (getline(&line, &length, file) != -1) {
        num_of_ops++;
        free(line);
        line = NULL;
    }
    free(line);
    // reset file pointer to first op
    if (fseek(file, saved_pos, SEEK_SET) != 0) {
        perror("fseek failed");
        // return EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    // calculate number of jobs per thread
    return num_of_ops / NUM_OF_WORKERS;
}

// populate account structs
void populate_acc(FILE *file) {
    // array of account structs
    ACC = (account **)malloc(sizeof(account *) * NUM_OF_ACC);
    for (int i = 0; i < NUM_OF_ACC; i++) {
        ACC[i] = (account *)malloc(sizeof(account));
        ACC[i]->transaction_tracker = 0;
        pthread_mutex_init(&(ACC[i]->ac_lock), NULL);
    }

    // reading file
    char *line = NULL;
    size_t length = 0;
    command_line line_parsed;

    // for indexing account were populating in ACC
    int acc_index = 0;
    // for mapping account info to struct
    int line_count = 0;
    // for mapping input account info line to struct
    int acc_map_i = 0;

    // loop over each input line
    // populating the account structs
    while (1) { 
        getline(&line, &length, file);
        // cleans up line
        line_parsed = str_filler(line, " ");
        // update the account index
        if (strcmp(line_parsed.command_list[0], "index") == 0) {
            acc_index = atoi(line_parsed.command_list[1]);
            acc_map_i = 0; // reset for next indexed account
            line_count = 0;
        }
        if (line_count == 1){
            strcpy(ACC[acc_index]->account_number, line_parsed.command_list[0]);
        }
        else if (line_count == 2){
            strcpy(ACC[acc_index]->password, line_parsed.command_list[0]);
        }
        else if (line_count == 3){
            ACC[acc_index]->balance = atof(line_parsed.command_list[0]);
        }
        else if (line_count == 4){
            ACC[acc_index]->reward_rate = atof(line_parsed.command_list[0]);
        }
        acc_map_i++;
        line_count++;

        free_command_line(&line_parsed);
        free(line);
        line = NULL;
        if (acc_index == (NUM_OF_ACC-1) && line_count == 5) {
            break;
        }
    }
}

void *process_transaction(void *arg) {

    // signaling main thread all workers are created
    pthread_barrier_wait(&barrier12);

    // this is a list of strings (list of ops)
    char **worker_ops = (char **)arg; // cast back to char **
    int op_counter = 0;
    command_line line_parsed;
    while(op_counter < JOBS_PER_WORKER) {
        // printf("%d\n", op_counter);
        line_parsed = str_filler(worker_ops[op_counter], " ");
        char acc_num[17];
        char pass[9];
        strcpy(acc_num, line_parsed.command_list[1]);
        strcpy(pass, line_parsed.command_list[2]);

        // find which account we're accessing
        int src_acc_exists = 0;
        int dst_acc_exists = 0; // for transfers
        int acc_src;
        int acc_dst;
        for (int i = 0; i < NUM_OF_ACC; i++) {
            // grab index of src account
            if (strcmp(ACC[i]->account_number, acc_num) == 0) {
                if (strcmp(ACC[i]->password, pass) == 0) {
                    src_acc_exists = 1;
                    acc_src = i;
                }
            }
            // if the op is transfer, grab dest account index
            if (strcmp(line_parsed.command_list[0], "T") == 0) {
                if (strcmp(ACC[i]->account_number, line_parsed.command_list[3]) == 0) {
                    dst_acc_exists = 1;
                    acc_dst = i;
                }
            }
        }
        // if account password pair doesn't exist return & checking is dst acount exists for transfer
        if ((src_acc_exists == 0) || (strcmp(line_parsed.command_list[0], "T") == 0 && dst_acc_exists == 0)){
            free_command_line(&line_parsed);
            op_counter++;
            continue;
        }

        complete_ops(&line_parsed, acc_src, acc_dst);

        free_command_line(&line_parsed);
        op_counter++;
    }
    return NULL;
}

void complete_ops(command_line *line_parsed, int acc_src, int acc_dst) {

    // to avoid race condition, lock smaller of src dst locks first
    if (strcmp((*line_parsed).command_list[0], "T") == 0) {

        pthread_mutex_lock(&transfer_mutex);
        pthread_mutex_lock(&(ACC[acc_src]->ac_lock)); 
        pthread_mutex_lock(&(ACC[acc_dst]->ac_lock)); 

        transfer(line_parsed, acc_src, acc_dst);

        pthread_mutex_unlock(&(ACC[acc_src]->ac_lock));
        pthread_mutex_unlock(&(ACC[acc_dst]->ac_lock));
        pthread_mutex_unlock(&transfer_mutex);

    } // withdraw
    else if (strcmp((*line_parsed).command_list[0], "W") == 0) {

        pthread_mutex_lock(&(ACC[acc_src]->ac_lock));
        withdraw(line_parsed, acc_src);
        pthread_mutex_unlock(&(ACC[acc_src]->ac_lock));

    } // deposit
    else if (strcmp((*line_parsed).command_list[0], "D") == 0) {

        pthread_mutex_lock(&(ACC[acc_src]->ac_lock));
        deposit(line_parsed, acc_src);
        pthread_mutex_unlock(&(ACC[acc_src]->ac_lock));

    } // check
    else if (strcmp((*line_parsed).command_list[0], "C") == 0) {
        // nothing to put here ...
    }

    if (strcmp((*line_parsed).command_list[0], "C") != 0) {
        // printf("worker locking transaction_count\n");
        pthread_mutex_lock(&transaction_count);
        NUM_OF_TRANSACTION += 1;
        if (NUM_OF_TRANSACTION % 5000 == 0) {
            pthread_mutex_lock(&bank_lock); // 3
            pthread_cond_signal(&bank_cond); // 4
            pthread_cond_wait(&worker_cond, &bank_lock); // 5 , 9
            pthread_mutex_unlock(&bank_lock);
        }
        pthread_mutex_unlock(&transaction_count);
    }
}

void transfer(command_line *line_parsed, int acc_src, int acc_dst) {
    double transfer_amount;
    char *endptr;
    transfer_amount = strtod((*line_parsed).command_list[4], &endptr); // potential error with NULL
    ACC[acc_src]->balance -= transfer_amount;
    ACC[acc_dst]->balance += transfer_amount;
    ACC[acc_src]->transaction_tracker += strtod((*line_parsed).command_list[4], &endptr);
}

void withdraw(command_line *line_parsed, int acc_src) {
    double transfer_amount;
    char *endptr;
    transfer_amount = strtod((*line_parsed).command_list[3], &endptr); // potential error with NULL
    ACC[acc_src]->balance -= transfer_amount;
    ACC[acc_src]->transaction_tracker += strtod((*line_parsed).command_list[3], &endptr);
}

void deposit(command_line *line_parsed, int acc_src) {
    double transfer_amount;
    char *endptr;
    transfer_amount = strtod((*line_parsed).command_list[3], &endptr); // potential error with NULL
    ACC[acc_src]->balance += transfer_amount;
    ACC[acc_src]->transaction_tracker += strtod((*line_parsed).command_list[3], &endptr);
}

// need a way to break out of this when all ops done!!!!!
// if NUM_OF_Transaction == num worekrs * jobs per worker
//      break
void *update_balance() {

    FILE *file;
    char *account_files[NUM_OF_WORKERS];
    int n;
    for (int i = 0; i < NUM_OF_WORKERS; i++) {
        char top_of_file[50];
        account_files[i] = malloc(30 * sizeof(char));
        if (account_files[i] == NULL) {
            perror("failed allocating account files");
            exit(EXIT_FAILURE);
        }
        n = snprintf(account_files[i], 30, "output/act_%d.txt", i);
        n = snprintf(top_of_file, sizeof(top_of_file), "account %d:\n", i);
        file = fopen(account_files[i], "w");
        fprintf(file, "%s", top_of_file);
        fclose(file);
    }

    pthread_mutex_lock(&bank_lock); // 1
    pthread_barrier_wait(&barrier12);

    while(1) {
        // printf("bank locking bank_lock\n");
        // printf("bank waiting for bank lock\n");
        pthread_cond_wait(&bank_cond, &bank_lock); // 2 , 6
        printf("bank applying rewards...\n");
        for (int i = 0; i < NUM_OF_ACC; i++) {
            ACC[i]->balance += ACC[i]->reward_rate * ACC[i]->transaction_tracker;
            ACC[i]->transaction_tracker = 0;
            file = fopen(account_files[i], "a");
            fprintf(file, "Current Balance:\t%.2f\n", ACC[i]->balance);
            fclose(file);
        }
        NUM_OF_UPDATES += 1;
        pthread_cond_signal(&worker_cond); // 7
        if (WORKERS_FINISHED) {
            break;
        }
    }
    for (int i = 0; i < 10; i++) {
        free(account_files[i]);
    }

    return &NUM_OF_UPDATES;
}