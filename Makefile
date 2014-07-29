

		
marcelino:
	gcc -o marcelino src/marcelino.c -lxcb
	
all:	marcelino

clean:	
	rm -fr *~
	rm -fr src/*~
	rm -f marcelino		