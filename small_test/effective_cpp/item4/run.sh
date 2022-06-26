g++ -c -o first.o first.cpp
g++ -c -o second.o second.cpp
g++ -c -o main.o main.cpp
g++ main.o first.o second.o
./a.out
echo ""
g++ main.o second.o first.o
./a.out
