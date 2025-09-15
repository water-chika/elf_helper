programs = get_elf_header bin2elf

all: ${programs}

bin2elf: bin2elf.cpp
	c++ bin2elf.cpp -o bin2elf -std=c++23

clean:
	rm -f ${programs}
