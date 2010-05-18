NAME=pysmbc
VERSION:=$(shell python setup.py --version)
SDIST_ARGS=--formats=bztar -d.

smbc.so: $(SOURCES)
	python setup.py build
	mv build/lib*/$@ .

doc: smbc.so
	rm -rf html
	epydoc -o html --html $<

clean:
	-rm -rf build smbc.so *.pyc tests/*.pyc *~ tests/*~

dist:
	python setup.py sdist $(SDIST_ARGS)

upload:
	python setup.py sdist $(SDIST_ARGS) upload -s

install:
	ROOT= ; \
	if [ -n "$$DESTDIR" ]; then ROOT="--root $$DESTDIR"; fi; \
	python setup.py install $$ROOT

.PHONY: doc clean dist install

