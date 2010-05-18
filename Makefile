NAME=pysmbc
VERSION:=$(shell python setup.py --version)

smbc.so: $(SOURCES)
	python setup.py build
	mv build/lib*/$@ .

doc: smbc.so
	rm -rf html
	epydoc -o html --html $<

clean:
	-rm -rf build smbc.so *.pyc *~

dist:
	python setup.py sdist --formats=bztar -d.

install:
	ROOT= ; \
	if [ -n "$$DESTDIR" ]; then ROOT="--root $$DESTDIR"; fi; \
	python setup.py install $$ROOT

.PHONY: doc clean dist install

