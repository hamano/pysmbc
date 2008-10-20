NAME=pysmbc
VERSION=1.0.6

SOURCES=smbcmodule.c smbcmodule.h context.c context.h dir.c dir.h \
	file.c file.h smbcdirent.c smbcdirent.h setup.py

DIST=Makefile test.py COPYING NEWS README TODO ChangeLog

smbc.so: $(SOURCES)
	python setup.py build
	mv build/lib*/$@ .

doc: smbc.so
	rm -rf html
	epydoc -o html --html $<

clean:
	-rm -rf build smbc.so *.pyc *~

dist:
	#svn export . $(NAME)
	mkdir $(NAME)-$(VERSION)
	#cd $(NAME); cp -a $(SOURCES) $(DIST) ../$(NAME)-$(VERSION); cd ..
	cp -a $(SOURCES) $(DIST) $(NAME)-$(VERSION)
	tar jcf $(NAME)-$(VERSION).tar.bz2 $(NAME)-$(VERSION)
	rm -rf $(NAME)-$(VERSION) $(NAME)

install:
	ROOT= ; \
	if [ -n "$$DESTDIR" ]; then ROOT="--root $$DESTDIR"; fi; \
	python setup.py install $$ROOT

.PHONY: doc clean dist install

