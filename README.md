# AudioEqualizer

Sistema de procesamiento de audio en tiempo real para Windows. Captura audio del sistema via WASAPI loopback y lo procesa a traves de una cadena DSP configurable antes de reenviarlo a un dispositivo de salida.

## Caracteristicas

- **Ecualizador parametrico** con N bandas configurables (low shelf, high shelf, peaking, etc.)
- **Compresor** con sidechain, knee, gate y expansion
- **Controles de tono** (bass/treble) para una mejora notoria apartir de un rango de freq
- **Crossover** sub-bass con HPF/LPF de pendiente variable (6/12/18/24/36/48 dB/oct)
- **Band Limiter** para limitar picos en bandas de un rango de frecuencias
- **Procesador multibanda** de 9 bandas con auto-balance, exciter y procesamiento
- **Reverb** basado en Freeverb (8 comb + 4 allpass filters) con pre-delay
- **Analizador espectral** en tiempo real

## Requisitos

- Windows 10/11
- con g++ (C++17)
- DirectX 11 SDK (incluido en Windows SDK)
- VOICE MEETER (https://vb-audio.com/Voicemeeter/index.htm)
- VB-CABLE Virtual Audio Device (https://vb-audio.com/Cable/)

## Compilacion

```bash
cd build
bash compile.sh
```

## Uso

1. Ejecutar `AudioEqualizer.exe`
2. Seleccionar dispositivo de captura (loopback) y dispositivo de salida
3. Ajustar los parametros DSP desde la interfaz grafica (La configuracion por defecto se carga automaticamente)
4. La configuracion se guarda/carga desde `config.json`
   
<img width="1919" height="1079" alt="Screenshot_1" src="https://github.com/user-attachments/assets/34639d6a-055f-4c46-8605-cae25be89e48" />
<img width="403" height="494" alt="Screenshot_4" src="https://github.com/user-attachments/assets/13117d82-ee5e-4a58-9991-de41ea961fce" />
<img width="1035" height="626" alt="Screenshot_3" src="https://github.com/user-attachments/assets/237ce55c-73d6-442d-8ecd-5bf914de7685" />
<img width="1087" height="128" alt="Screenshot_5" src="https://github.com/user-attachments/assets/5f55a0b9-5f83-4538-80eb-4629aab69881" />
<img width="1136" height="705" alt="Screenshot_2" src="https://github.com/user-attachments/assets/58b136ca-1366-465f-8fb8-554df1417c98" />
