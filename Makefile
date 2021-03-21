CC=g++
CFLAGS=-Wall -std=c++11 -ggdb3

all: date.o date_tree.o hash_table.o patientRecord.o lists.o signals_worker.o signals_master.o main_worker.o worker master whoServer whoClient circular_buffer.o whoServer_main.o

lists.o: lists.cpp lists.h
	$(CC) $(CFLAGS) -c lists.cpp

date.o: date.cpp date.h
	$(CC) $(CFLAGS) -c date.cpp

date_tree.o: date_tree.cpp date_tree.h
	$(CC) $(CFLAGS) -c date_tree.cpp

hash_table.o: hash_table.cpp hash_table.h
	$(CC) $(CFLAGS) -c hash_table.cpp

patientRecord.o: patientRecord.cpp patientRecord.h
	$(CC) $(CFLAGS) -c patientRecord.cpp

signals_worker.o: signals_worker.cpp signals_worker.h
	$(CC) $(CFLAGS) -c signals_worker.cpp

signals_master.o: signals_master.cpp signals_master.h
	$(CC) $(CFLAGS) -c signals_master.cpp

main_worker.o: main_worker.cpp
	$(CC) $(CFLAGS) -c main_worker.cpp

worker: main_worker.o lists.o signals_worker.o patientRecord.o hash_table.o date_tree.o date.o
	$(CC) $(CFLAGS) -o worker main_worker.o hash_table.o patientRecord.o date_tree.o date.o lists.o signals_worker.o

master: master.cpp signals_master.cpp
	$(CC) $(CFLAGS) -o master master.cpp signals_master.cpp

whoServer_main.o: whoServer_main.cpp
	$(CC) $(CFLAGS) -c whoServer_main.cpp

whoServer: whoServer_main.o circular_buffer.o lists.o
	$(CC) $(CFLAGS) -pthread -o whoServer whoServer_main.o circular_buffer.o lists.o

whoClient: whoClient_main.cpp circular_buffer.o
	$(CC) $(CFLAGS) -pthread -o whoClient whoClient_main.cpp

circular_buffer.o: circular_buffer.cpp circular_buffer.h
	$(CC) $(CFLAGS) -c circular_buffer.cpp

.PHONY: clean

clean:
	rm -f worker main_worker.o hash_table.o patientRecord.o date_tree.o date.o lists.o signals_worker.o signals_master.o master whoServer whoClient circular_buffer.o whoServer_main.o
