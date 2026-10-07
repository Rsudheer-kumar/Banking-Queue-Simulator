FROM ubuntu:24.04 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY backend /src/backend

RUN cmake -S /src/backend -B /src/backend/build \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /src/backend/build --target bank_server

FROM ubuntu:24.04

WORKDIR /app
COPY --from=build /src/backend/build/bank_server /app/bank_server

ENV PORT=8080
EXPOSE 8080

CMD ["/bin/sh", "-c", "exec /app/bank_server"]
