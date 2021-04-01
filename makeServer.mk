all:
	gcc6 -pthread server.cpp -o server -lstdc++
	server config.input
