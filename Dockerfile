FROM ubuntu:20.04

WORKDIR /app

RUN apt-get update \
    && apt-get -y full-upgrade \
    && apt-get install tzdata software-properties-common -y \
    && add-apt-repository 'ppa:deadsnakes/ppa' \
    && apt-get update \
    && apt-get -y install python3.9

RUN apt-get install -y \
    python3.9 \
    python3.9-venv \
    python3-pip

COPY . .

RUN pip install .

CMD ["uncalled"]