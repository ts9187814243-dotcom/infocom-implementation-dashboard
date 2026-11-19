FROM ubuntu:22.04 AS builder
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ make curl ca-certificates libpqxx-dev libpq-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
RUN curl -fsSL https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h -o httplib.h
COPY app/ ./app/
COPY include/ ./include/
RUN g++ -std=c++17 -O2 -Wall -Wextra \
    -I/app -I/app/include \
    /app/app/main.cpp -o /app/server \
    -lpqxx -lpq -pthread

FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 libpqxx-6.4 ca-certificates postgresql-client \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/server /app/server
COPY migrate.sh /app/migrate.sh
COPY migrations/ /app/migrations/
RUN chmod +x /app/migrate.sh
EXPOSE 8080
CMD ["/app/migrate.sh"]
