# Stage 1: Build stage
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project files into builder
COPY . .

# Configure and compile release binary
RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    && cmake --build build --target todo --parallel

# Stage 2: Minimal runtime image
FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime standard C++ libraries and CA certificates
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Run as non-privileged user
RUN useradd -m -s /bin/bash todouser
USER todouser
WORKDIR /home/todouser

# Copy compiled binary from builder
COPY --from=builder /app/build/todo /usr/local/bin/todo

ENTRYPOINT ["todo"]
CMD ["--help"]
