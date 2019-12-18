## first semester work - shell
##### Examples of input commands:
    * simple command:    ls -l
    * output redirection: ls -l > ouput.txt
    * input redirection: grep "HELP PLZ" < input.txt
    * pipeline for n variable: ls -l | grep .c |
    * background mode: firefox &
    * change directory: cd ../bash
##### To compile a program user can download Makefile and create makefile with this parametr:
    * %: %.c
	     gcc $@.c -o $@ -Wall -Werror -lm -fsanitize=leak,undefined,address
##### To run program user need to use command: ./shell
