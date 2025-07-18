FILES= code/

XDFILE = code/data 
XCFILE = code/capture
XRFILE = code/regression-cache

handin.tar: clean
	tar cvf handin.tar --exclude=$(XDFILE) --exclude=$(XCFILE) --exclude=$(XRFILE) $(FILES) 

clean:
	(cd code; make clean)
	rm -f *~ handin.tar
