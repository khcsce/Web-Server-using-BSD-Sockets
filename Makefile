# NAME: Khoa Quach
# EMAIL: khoaquachschool@gmail.com
# ID: 105123806
CC = gcc
CFLAGS = -Wall -g
UID=105123806

default:
	$(CC) $(CFLAGS) -lm -o webserver webserver.c

clean: 
	rm -f *.tar.gz *.o *~ webserver

dist: clean
	tar -cvzf $(UID).tar.gz webserver.c README Makefile *.png *.jpg *.jpeg *.html *.gif *.txt binaryfile downloadfile
