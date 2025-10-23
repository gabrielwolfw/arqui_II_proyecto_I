
Interfaz que arranca el proyecto y ejecuta `make run` y muestra en tiempo real stdout y stderr en una ventana. Está pensado exclusivamente como medio de interacción/visualización

Requisitos
- Python 3.7+
- Tkinter (en Debian/Ubuntu: `sudo apt-get install python3-tk`)

Uso
1. Desde la raíz del repositorio ejecutar:
   ```bash
   python3 src/gui/gui.py
   ```
   - La GUI, por defecto, ejecutará `make run` en la raíz del repo.
2. Controles:
   - Iniciar: lanza el comando especificado (por defecto `make run`).
   - Detener: intenta terminar el proceso.
   - Borrar: limpia la vista.
   - Guardar log: exporta la salida mostrada a un archivo .log o .txt.
