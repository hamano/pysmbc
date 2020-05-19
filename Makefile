PYTHON=python
NAME=pysmbc
VERSION:=$(shell $(PYTHON) setup.py --version)

_smbc.so: force
	$(PYTHON) setup.py build
	mv build/lib*/_smbc*.so .

doc: _smbc.so
	rm -rf html
	epydoc -o html --html $<

doczip:	doc
	cd html && zip ../smbc-html.zip *

clean:
	-rm -rf build smbc.so *.pyc tests/*.pyc *~ tests/*~ _smbc*.so 

dist:
	$(PYTHON) setup.py sdist $(SDIST_ARGS)

upload:
	$(PYTHON) setup.py sdist $(SDIST_ARGS) upload -s

install:
	ROOT= ; \
	if [ -n "$$DESTDIR" ]; then ROOT="--root $$DESTDIR"; fi; \
	$(PYTHON) setup.py install $$ROOT

.PHONY: doc doczip clean dist install force

