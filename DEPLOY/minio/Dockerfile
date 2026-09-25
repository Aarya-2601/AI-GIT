FROM golang:1.24-alpine AS builder

RUN apk add --no-cache git ca-certificates

WORKDIR /src

RUN git clone --depth 1 https://github.com/minio/minio.git .

RUN CGO_ENABLED=0 go build -o /minio .

FROM alpine:3.22

RUN apk add --no-cache ca-certificates

COPY --from=builder /minio /usr/local/bin/minio

EXPOSE 9000 9001

ENTRYPOINT ["minio"]

CMD ["server", "/data", "--console-address", ":9001"]