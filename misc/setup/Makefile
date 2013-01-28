ifndef VERSION
VERSION=0.7
endif
ifndef RELEASE
RELEASE=0
endif

all:
	VERSION=$(VERSION) RELEASE=$(RELEASE) ./doit

sign:
	for i in *.run; do \
		gpg -bao $$i.asc $$i; \
	done

clean:
	rm -rf *.run image defines.m4

.PHONY: all sign clean
