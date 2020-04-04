FROM debian

RUN apt update && apt -y install git make gcc zlib1g-dev

RUN git clone https://github.com/sebdel/mra-tools-c.git
RUN cd mra-tools-c && make
COPY entry.sh mra-tools-c/entry.sh

WORKDIR /mras

ENTRYPOINT ["../mra-tools-c/entry.sh"]
