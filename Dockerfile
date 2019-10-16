FROM alpine:latest as builder

RUN apk add --no-cache build-base

WORKDIR /app

COPY src src
COPY makefile makefile

RUN make

FROM alpine:latest

WORKDIR /app

COPY --from=builder /app/uzlog .

EXPOSE 7000/udp

CMD ["./uzlog", "-fco", "-p 7000"]