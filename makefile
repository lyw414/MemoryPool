all:
	g++ -std=c++11 -O2 -o Test test.cpp MemoryPool.cpp -lpthread -I ./
debug:
	g++ -g -o Test test.cpp MemoryPool.cpp -lpthread -I ./
