all : bank

bank : bank.o string_parser.o
	gcc -g -pthread -o bank bank.o string_parser.o

bank.o: bank.c account.h bank.h
	gcc -g -pthread -c bank.c

string_parser.o: string_parser.c string_parser.h
	gcc -g -pthread -c string_parser.c

clean:
	rm -f core *.o output/output.txt output/act_*.txt bank
