FROM gcc:13 AS builder

WORKDIR /app

# Скачиваем заголовочный файл httplib.h
RUN curl -sSL -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

# Копируем исходный код бэкенда из подпапки app
COPY app/ ./

# Компилируем C++ сервер
RUN g++ -O2 main.cpp -o server -lpthread

# Финальный легкий образ
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y libstdc++6 ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/server .

EXPOSE 8080

CMD ["./server"]
