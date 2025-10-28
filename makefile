all:
	gcc -Wall -Wextra -O2 -g3 -pthread log_analyser_seq.c -o log_analyser_seq
	gcc -Wall -Wextra -O2 -g3 -pthread log_analyser_par.c -o log_analyser_par

clean:
	rm -f log_analyser_seq log_analyser_par
