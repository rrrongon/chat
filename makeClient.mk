all:
	gcc6 client.cpp -o client -lstdc++
	client c_config.input
