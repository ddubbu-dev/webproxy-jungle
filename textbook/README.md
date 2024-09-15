# 11.4.9 Example Echo Client and Server
## Intro
> learn socket interface

1. After establishing a connection with the server
2. the client enters a loop that repeatedly reads a text line from standard input
3. (client) sends the text line to the server
4. (client) reads the echo line from the server and prints the result to standard output


## Code
### common
- `$ cd textbook`

### server
- 컴파일 및 링크
`$ gcc -I.. echo_server.c ../csapp.c -o echo_server.exe`

- 실행
`$ ./echo_server.exe 8080`

### client
- 컴파일 및 링크
`$ gcc -I.. echo_client.c ../csapp.c -o echo_client.exe`

- 실행
`$ ./echo_client.exe 127.0.0.1 8080`
