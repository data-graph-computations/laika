FROM ubuntu:18.04
# Ubuntu 18.04 ships with GCC 7, which is sufficiently new for Laika to work.

ADD ./scripts /laika/scripts
RUN chmod +x /laika/scripts/aws_setup.sh && \
    /laika/scripts/aws_setup.sh && \
    chmod +x /laika/scripts/download_input_data.sh && \
    /laika/scripts/download_input_data.sh

ADD . /laika

WORKDIR /laika
