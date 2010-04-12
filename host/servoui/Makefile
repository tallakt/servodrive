OUTNAME=servoui

all: $(OUTNAME)

$(OUTNAME): $(OUTNAME).cpp
	g++ $^ -o $@ -Wall -g -I ../..

clean:
	-rm $(OUTNAME)

copy:
	cp $(OUTNAME) ../..
