output: solitaire.o
	gcc solitaire.o -o solitaire
	
solitaire.o: solitaire.c
	gcc -c solitaire.c
	
start:
	./solitaire config.txt

clean:
	rm *.o solitaire
