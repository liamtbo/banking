# Multithreaded Banking
![](images/banking.jpg)

## Overview

This banking project is a multi-threaded system that simulates banking operations such as deposits, withdrawals, and transfers between multiple accounts. The project utilizes POSIX threads (pthreads) for concurrency, allowing multiple worker threads to process transactions in parallel and maintains thread safety through the use of mutexes and barriers.

The program reads account and transaction information from an input file, processes the transactions concurrently across multiple worker threads, and applies updates to account balances. A separate bank thread calculates rewards for account holders at each 5000 transactions completed mark.
## Features

- Multi-threaded design using pthreads.
- Support for deposit, withdrawal, and transfer transactions.
- Safe concurrent access to shared account data with mutex locking.
- Job distribution among worker threads for parallel processing.
- Synchronization of worker and bank threads with pthread barriers.
- File I/O for reading accounts and transactions, and writing updated account- balances to an output file.

## Project Structure

- bank.h: Header file for bank-related functions and global variables.
- string_parser.h: Utility for parsing strings into commands for transactions.
- account.h: Defines the structure for the bank's accounts.
- main.c: Contains the main function and thread logic.

## How It Works

- Input Parsing: The program begins by parsing the number of accounts and initializing the account structures from the input file. It then assigns a set of transactions to each worker thread for processing.

- Worker Threads: Each worker thread processes a set number of transactions (withdrawals, deposits, transfers) concurrently. Mutexes ensure that access to account data is thread-safe.

- Bank Thread: A separate thread is responsible for updating the balances of the accounts at the end of all transactions, applying any rewards.

- Synchronization: The program uses pthread barriers to synchronize worker threads and ensure that the bank thread only runs after all workers have completed their tasks.

- Output: Once all transactions and updates are complete, the program writes the final balances of each account to an output file.

## How to Build and Run

### Prerequisites

- GCC compiler (for compiling C programs).
- POSIX-compliant system for pthread support.

### Usage

To run the program, use the following command:

    ./runner.sh

This will create 10 worker threads and process transactions from input.txt.

### Input File Format

The input file follows this format:

- The first line contains the number of accounts.
- For each account, there are five lines:
  - Account index
  - Account number
  - Account password
  - Account balance
  - Reward rate
- After the accounts, transactions are listed:
  - Transaction type (D for deposit, W for withdrawal, T for transfer)
  - Source account number
  - Password
  - Amount (or destination account number for transfer transactions)

### Output

The final account balances are written to output/output.txt in the following format:

    <account_index> balance: <final_balance>

## Thread Synchronization and Safety

- Mutexes: Ensure safe access to shared account data.
- Barriers: Synchronize the main thread with worker threads and the bank thread.
- Conditions: Used for signaling between threads.

## Cleaning Up

At the end of the program:

- All mutexes are destroyed.
- All allocated memory (for accounts and worker job arrays) is freed.
- The output file is closed.