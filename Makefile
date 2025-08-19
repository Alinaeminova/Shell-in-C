myshell: executor.o parser.o lexer.o
	gcc -o myshell executor.o parser.o lexer.o
	rm -f *.o

executor.o: executor.c parser.h
	gcc -c executor.c

parser.o: parser.c lexer.h
	gcc -c parser.c

lexer.o: lexer.c lexer.h
	gcc -c lexer.c

clean:
	rm -f *.o myshell