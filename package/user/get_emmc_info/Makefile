SRC		=	$(wildcard ./src/*.c)
PRJ_LIBS	= -lpthread
OBJS	=	$(SRC:%.c=%.o)

CFLAGS	= -I ./inc $(ALL_INC) -g -Wall -lpthread

all:  emmc_get_info

emmc_get_info:$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(PRJ_LIBS)
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	-rm -f *.o
	-rm -f $(wildcard ./*.o) $(wildcard ./**/*.o) $(wildcard ./**/**/*.o)

