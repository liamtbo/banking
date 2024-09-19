#ifndef BANK_H_
#define BANK_H_

#include "account.h"
#include "string_parser.h"
#include <stdio.h>

void populate_acc(FILE *file);
void *process_transaction(void *arg);
void *update_balance();
long num_jobs_per_worker(FILE *file, char *line, size_t length);
void assigning_workers_ops(FILE *file, char ****workers_ops, int jobs_per_worker);
void complete_ops(command_line *str_parser, int acc_src, int acc_dst);

void transfer(command_line *line_parsed, int acc_src, int acc_dst);
void withdraw(command_line *line_parsed, int acc_src);
void deposit(command_line *line_parsed, int acc_src);

#endif