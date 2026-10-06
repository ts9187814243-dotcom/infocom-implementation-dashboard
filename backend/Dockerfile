FROM gcc::13 AS builder
WORKDIR /app
RUN curl -0 https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h
COPY app/main.cpp .
RUN g++ -02 -static -o server main.cpp

FROM scratch
COPY --from=builder /app/server /server
EXPOSE 8000
CMD ["/server"]