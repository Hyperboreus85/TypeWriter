# TypeWriter ESP32 (PlatformIO)

Firmware per ESP32 con interfaccia a **encoder + pulsante** e display OLED **SH1106 1.3" 128x64 I2C**.

Il progetto permette di:
- impostare il ritardo carattere e pausa riga,
- modificare una stringa da “digitare” in seriale,
- usare un piccolo menu segreto protetto da password,
- mostrare UI su OLED.

## Requisiti software

- [PlatformIO](https://platformio.org/) (CLI o estensione VS Code)
- Scheda: `esp32dev`
- Framework: Arduino

Dipendenze (`platformio.ini`):
- `adafruit/Adafruit GFX Library`
- `adafruit/Adafruit SH110X`

## Collegamenti hardware (ESP32)

> ⚠️ Alimentazione: verifica sempre la tensione del tuo modulo OLED/encoder. In generale su ESP32 è consigliato usare **3.3V** per segnali logici sicuri.

### 1) Display OLED SH1106 (I2C, 128x64)

Nel firmware:
- SDA = GPIO **21**
- SCL = GPIO **22**
- clock I2C = **100 kHz**
- reset display = **non collegato** (`-1`)
- address default = **0x3C** (alcuni moduli usano **0x3D**)

| OLED SH1106 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| RES/RST | NC (non collegato) |

### 2) Encoder rotativo (A/B + push)

Nel firmware:
- Encoder A = GPIO **32**
- Encoder B = GPIO **33**
- Pulsante = GPIO **25**

| Modulo encoder | ESP32 |
|---|---|
| CLK / A | GPIO 32 |
| DT / B | GPIO 33 |
| SW | GPIO 25 |
| + | 3V3 |
| GND | GND |

## Schema rapido collegamenti

- **OLED SH1106 I2C** → `GPIO21 (SDA)`, `GPIO22 (SCL)`, `3V3`, `GND`
- **Encoder** → `GPIO32 (A)`, `GPIO33 (B)`, `GPIO25 (SW)`, `3V3`, `GND`

## Build e upload

Da root progetto:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## Comportamento UI

- **Main screen**: mostra timing e testo corrente.
- **Click breve**: alterna modifica `CHAR` / `PAUSA`.
- **Doppio click**: entra nell’editor stringa.
- **Pressione lunga (5s)**: entra nel menu password.

## Note SH1106

Se il display non viene rilevato:
1. controlla cablaggio SDA/SCL,
2. prova address I2C `0x3D` al posto di `0x3C`,
3. verifica massa comune e alimentazione stabile.

## Licenza

Vedi `LICENSE`.
