:
- server code: server.cpp
- client code: client.cpp
- server config file: config.input
- client config file: c_config.input

## commands to build and run:
```sh
make -f makeClient.mk
make -f makeServer.mk
```
## Behave:
- server starts and show at which port it is listening
- client starts, reads c_cofing file and connects accordingly.
- I/O multiplexing has been achieved in client side by implementing Select()
- Server side concurrent server has been achieved by implementing pthread.
- When needed for variables used pthread lock and unlock
- SIGINT handeled
- Chatting to one user and broadcast

## Commands and response:
```sh
login USER_NAME
```
this creats an entry for the user with username. Now client is logged in state. Client now can send messages to a particular user or can broadcast messages to all.
```sh
chat MSG
```
this command broadcasts messages to all the logged in users.

```sh
chat @USERNAME MSG
```
this command sends message to the particular USERNAME user.

```sh
-> USER_NAME >> MESSAGE THAT USER_NAME SENT
```
When any user receives any message it sees as above.

```sh
exit
```
exit command in client side first logsout from server. Then, closes the opened socket and exnds client process.

```sh
logout
```
Logout logs out user from the server. It will no loger receive or send any messages to server.

```sh
ctrl+C
```
This has been handeled as SIGINT signal. When, receive any SIGINT signal, client logs out first. closes the socket.
If any SIGINT is received in server side, then Server closes all sockets of clients and then exits from the main process. Server is shut.

## Technical thigns covered:
This project has been covered all the required commands and behaves of:
```sh
- proper README file and makefile
- program initialization with configuration file
- user commands login, logout, and exit 
- client chat command "chat" 
- handling SIGINT signal 
- usage of pthread 
- usage of select()
```

# Author:
Rubayet Rahman Rongon
