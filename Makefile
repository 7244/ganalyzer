OUTPUT = a.exe

LINK = -lm -lfftw3
INCLUDE = 

debug:
	lclang++ -g main.cpp -o $(OUTPUT) $(INCLUDE) $(LINK)

release:
	lclang++ -s -O3 main.cpp -o $(OUTPUT) $(INCLUDE) $(LINK)

clean:
	rm $(OUTPUT)
