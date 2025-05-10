FROM debian:12

RUN apt-get update && \
    apt-get install -y build-essential valgrind man-db

WORKDIR /workspace

COPY . .

CMD ["bash"]
