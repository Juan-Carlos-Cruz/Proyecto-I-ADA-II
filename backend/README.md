# Backend C++

Este directorio contiene el backend provisional en C++ que expone el servidor HTTP para la interfaz.

## Arranque recomendado

Desde la raiz del proyecto:

```bash
./iniciar_proyecto
```

Eso compila el backend, lo levanta en `http://127.0.0.1:8080` y deja la interfaz lista para usarse.

Para detenerlo:

```bash
./detener_proyecto
```

## Estructura sugerida

```text
backend/
├── include/   # Headers publicos, tipos y contratos compartidos
├── src/       # Servidor HTTP, parser JSON provisional y stub de calculo
├── tests/     # Pruebas unitarias e integracion
└── logs/      # Logs generados por el lanzador (se crea en ejecucion)
```

## Contrato esperado por el frontend

Endpoint sugerido:

```http
POST /calcular
Content-Type: application/json
```

Request:

```json
{
  "algoritmo": "roFB",
  "finca": "5\n10,3,4,0\n6,3,3,1\n2,2,1,0\n8,1,1,6\n10,4,2,6"
}
```

Response:

```json
{
  "costo": 42,
  "permutacion": [0, 2, 1, 4, 3],
  "detalles": [
    { "tablon": 0, "inicio": 0, "caso": 1, "costo": 5 }
  ]
}
```

## Nota de integracion

El frontend ya tiene una configuracion central en `assets/js/config/app-config.js`. Cuando el backend exista, basta con:

1. Mantener `baseUrl` y `calculatePath`, o ajustarlos si cambias el puerto/ruta.
2. Reemplazar la logica provisional en `backend/src/placeholder_engine.cpp`.
3. Mantener el contrato JSON indicado arriba para no tocar el frontend.

## Estado actual

- El servidor ya responde `GET /health`.
- El servidor ya responde `OPTIONS /calcular` con CORS listo para navegador.
- El servidor ya responde `POST /calcular` con una salida provisional funcional.
- La logica de algoritmos todavia es un stub y debe reemplazarse por tu implementacion real.
- Cada request se atiende en un proceso hijo, para que un fallo puntual del algoritmo no tumbe el proceso principal del servidor.
- El arranque desde `./iniciar_proyecto` usa un supervisor local que relanza el backend si el proceso principal se cae.

## Donde escribir los algoritmos

Para mantener separado lo complicado del servidor y lo sencillo de la logica:

- `backend/src/http_server.cpp`: no tocar, aqui vive HTTP/sockets/CORS.
- `backend/src/main.cpp`: no tocar, aqui vive el arranque del backend.
- `backend/src/placeholder_engine.cpp`: parser y puente entre request y algoritmos.
- `backend/src/algoritmos.cpp`: aqui debes escribir tus algoritmos.

La idea es que en `algoritmos.cpp` programes con estilo simple de clase, usando `vector`, `sort`, `next_permutation`, ciclos y condicionales normales, sin preocuparte por HTTP ni JSON.

Los tipos base para la logica quedaron pensados para leerse facil:

- `Tablon`: datos de entrada de cada zona de riego.
- `EntradaCalculo`: lo que llega desde la interfaz.
- `DetalleRiego`: informacion puntual de cada tablon en la solucion.
- `ResultadoCalculo`: lo que el algoritmo devuelve al frontend.

## Como levantar el backend

Opcion recomendada:

```bash
./iniciar_proyecto
```

Eso:

1. compila el backend
2. lo levanta en `http://127.0.0.1:8080`
3. deja listo el frontend

Si no quieres que abra el frontend automaticamente:

```bash
./iniciar_proyecto --no-open
```

Para detener el backend:

```bash
./detener_proyecto
```

Si quieres levantar solo el backend manualmente:

```bash
make backend
./backend/bin/riego_backend --port 8080
```
