# Arquitectura del proyecto

## Estructura actual

```text
.
├── assets/
│   ├── css/
│   │   ├── base.css
│   │   ├── components.css
│   │   ├── layout.css
│   │   ├── responsive.css
│   │   └── tokens.css
│   └── js/
│       ├── app.js
│       ├── config/
│       │   └── app-config.js
│       ├── core/
│       │   └── state.js
│       ├── services/
│       │   ├── api-client.js
│       │   ├── export-service.js
│       │   └── file-parser.js
│       └── ui/
│           ├── config-screen.js
│           ├── navigation.js
│           └── result-screen.js
├── backend/
│   ├── include/
│   │   ├── domain_types.hpp
│   │   ├── http_server.hpp
│   │   ├── json_utils.hpp
│   │   └── placeholder_engine.hpp
│   ├── logs/
│   ├── src/
│   │   ├── http_server.cpp
│   │   ├── json_utils.cpp
│   │   ├── main.cpp
│   │   └── placeholder_engine.cpp
│   └── tests/
├── docs/
│   └── arquitectura-proyecto.md
├── F1.txt
├── Makefile
├── iniciar_proyecto
├── detener_proyecto
├── riego_interfaz_frontend.html
└── riego_optimo_interfaz.html
```

## Criterios de organizacion

- `assets/css/`: estilos divididos por capas, no por pagina.
- `assets/js/config/`: configuracion editable sin tocar la logica.
- `assets/js/core/`: estado compartido de la aplicacion.
- `assets/js/services/`: integraciones y reglas de infraestructura.
- `assets/js/ui/`: renderizado y comportamiento de pantallas.
- `assets/js/app.js`: punto de entrada que coordina los modulos.
- `backend/`: servicio C++ provisional, listo para reemplazar el stub por logica real.
- `Makefile`: compilacion simple del backend sin depender de `cmake`.
- `iniciar_proyecto`: lanzador raiz que compila y arranca el backend automaticamente.
- `detener_proyecto`: cierre controlado del backend levantado por el lanzador.

## Flujo de integracion con backend

1. El usuario carga un `.txt`.
2. `file-parser.js` valida y transforma el archivo en memoria.
3. `app.js` coordina la ejecucion.
4. `api-client.js` envia el request al backend C++.
5. `result-screen.js` pinta la respuesta.
6. `export-service.js` permite descargar la salida `.txt`.
7. `iniciar_proyecto` automatiza el backend para que el usuario no tenga que montar localhost manualmente.

## Decisiones practicas

- Se mantuvo `riego_interfaz_frontend.html` como entrypoint para no romper tu flujo actual.
- La configuracion del backend vive en un solo lugar.
- La UI no conoce detalles de `fetch`; eso queda aislado en `api-client.js`.
- La exportacion `.txt` tampoco depende de la pantalla; queda como servicio reutilizable.
- El backend actual es estable para integracion y pruebas del frontend, pero la logica de optimizacion debe reemplazarse en `placeholder_engine.cpp`.
