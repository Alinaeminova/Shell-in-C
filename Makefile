myshell: parser.o lexer.o
	gcc -o myshell parser.o lexer.o
	rm -f *.o

parser.o: parser.c lexer.h
	gcc -c parser.c

lexer.o: lexer.c lexer.h
	gcc -c lexer.c

clean:
	rm -f *.o myshell