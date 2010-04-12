OUTNAME=servoctrl

all: $(OUTNAME)

$(OUTNAME): $(OUTNAME).cpp
	arm-none-linux-gnueabi-g++ $^ -o $@ -Wall -I /home/chrisd/source/linux-beagle-2.6/include -I ../..

clean:
	-rm $(OUTNAME)

copy:
	sudo cp $(OUTNAME) /media/$(DISK)/home/root
