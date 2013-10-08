.PHONY: all install

install : all install.sh
	./install.sh

install.sh :
	./setup --install --no-install-interactive

all : *.c
	./setup



