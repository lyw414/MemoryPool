all:
	g++ -Wall -O2 -std=c++98 -o Test test.cpp MemoryPool.cpp Register.cpp -lpthread -I ./
debug:
	g++ -g -Wall -std=c++98 -o Test test.cpp MemoryPool.cpp Register.cpp -lpthread -I ./
