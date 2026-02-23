FROM debian:13
COPY lib lib
COPY GhostServer GhostServer
COPY Makefile Makefile
RUN apt-get update
RUN apt-get install -y build-essential gcc make
RUN make ghost_server_cli
EXPOSE 53000
ENTRYPOINT ["./ghost_server_cli"]
