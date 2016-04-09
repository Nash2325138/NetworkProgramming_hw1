CC=gcc -Wall
ser_c=HW1_103062224_Ser.c
cli_c=HW1_103062224_Cli.c
client: ${cli_c}
	${CC} -o client ${cli_c} 
server: ${ser_c}
	${CC} -o server ${ser_c}
all: ${ser_c} ${cli_c}
	${CC} -o client ${cli_c}
	${CC} -o server ${ser_c}
