# include/

Ubica aqui los headers publicos del backend, por ejemplo:

- modelos de dominio
- interfaces de servicio
- utilidades compartidas

g++ -std=c++20 -Ibackend/include backend/src/algoritmos.cpp prueba.cpp -o prueba // Usar esto para hacer pruebas