# **Dashprom**

Este proyecto implementa un sistema distribuido para monitorizar las métricas de rendimiento de un sistema, incluyendo:

- **Uso de CPU**
- **Uso de RAM**
- **Uso de disco**
- **Promedio de carga (Load Average)**
- **Número de usuarios conectados**
- **Número de procesos activos**

El sistema consta de un servidor, clientes y una herramienta para pruebas de esfuerzo (stress test).

---

## **Compilación**

Para compilar el proyecto, utiliza el comando:

```bash
make
```

## **Ejecución**
1. Ejecutar el servidor

Inicia el servidor especificando el número de clientes que esperas conectar:
Para compilar el proyecto, utiliza el comando:

```bash
./servidor <numero_clientes>
```
Por ejemplo:
```bash
./servidor 5
```

2. Conectar clientes

Cada cliente se conecta al servidor ejecutando:
```bash
./cliente <ip_servidor> 1234
```

Por ejemplo:
```bash
./cliente 192.168.1.100 1234
```

3. Pruebas de esfuerzo

Para evaluar el rendimiento del sistema, ejecuta la prueba de esfuerzo:
```bash
./prueba_stress
```

## **Arquitectura**

### *Agente*

El componente agente recolecta métricas del sistema utilizando comandos del sistema operativo y las envía al cliente. Las métricas recolectadas incluyen:

- CPU: Uso porcentual de la CPU calculado a partir de /proc/stat.

- RAM: Porcentaje de uso de memoria basado en la salida del comando free.

- Disco: Uso de disco en porcentaje extraído de df.

- Promedio de carga: Valor obtenido de /proc/loadavg.

- Usuarios: Número de usuarios conectados basado en el comando who.

- Procesos: Conteo total de procesos activos usando ps.

### *Cliente*

El cliente recibe las métricas del agente, genera alertas si se superan ciertos umbrales y las envía al servidor. Además, presenta un dashboard en tiempo real para monitorizar el estado del sistema.

### *Servidor*

El servidor gestiona las conexiones de múltiples clientes, almacena las métricas recibidas y genera alertas para cada cliente si se superan umbrales críticos. Las alertas pueden enviarse vía servicios como WhatsApp.

### *Prueba de Stress*

El programa prueba_stress genera cargas intensivas en:

CPU: Operaciones matemáticas continuas.

RAM: Asignación y escritura en bloques grandes de memoria.

Disco: Escritura repetitiva de datos en un archivo.

