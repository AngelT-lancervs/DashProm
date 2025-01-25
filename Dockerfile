# Dockerfile para contenerizar el servidor

# Usa una imagen base con soporte para C
FROM debian:bullseye

# Instalar dependencias necesarias
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copiar los archivos del servidor al contenedor
WORKDIR /app
COPY servidor.c ./

# Compilar el servidor
RUN gcc -o servidor servidor.c -lpthread -lcurl

# Exponer el puerto del servidor
EXPOSE 1234

# Comando para ejecutar el servidor
CMD ["./servidor", "5"]
