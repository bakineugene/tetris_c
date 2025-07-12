import wave
import sys

def wav_to_progmem(sound_name):
    try:
        with wave.open("sounds/" + sound_name + ".wav", 'rb') as wav:
            params = wav.getparams()
            
            if params.nchannels != 1:
                raise ValueError("Только моно-файлы!")
            if params.sampwidth != 1:
                raise ValueError("Поддерживаются только 8-битные!")
            
            data = wav.readframes(params.nframes)
            
            with open("build/sounds/" + sound_name + ".h", 'w') as f:
                f.write(f"#define SOUND_{sound_name.upper()}_LENGTH {len(data)}\n")
                f.write(f"#if defined(__AVR__)\n")
                f.write(f"const uint8_t SOUND_{sound_name.upper()}_DATA[SOUND_{sound_name.upper()}_LENGTH] PROGMEM = {{\n")
                f.write(f"#else\n")
                f.write(f"const uint8_t SOUND_{sound_name.upper()}_DATA[SOUND_{sound_name.upper()}_LENGTH] = {{\n")
                f.write(f"#endif\n")
                
                for i in range(0, len(data), 16):
                    chunk = data[i:i+16]
                    hex_values = ', '.join(f'0x{b:02X}' for b in chunk)
                    f.write(f" {hex_values},\n")
                
                f.write("};\n\n")
                f.write(f"const Sound SOUND_{sound_name.upper()} = {{ SOUND_{sound_name.upper()}_LENGTH, SOUND_{sound_name.upper()}_DATA }};\n")
            
            print(f"Успешно конвертировано: {len(data)} байт")
            print(f"Частота дискретизации: {params.framerate} Гц")
            print(f"Длительность: {len(data)/params.framerate:.2f} сек")
    
    except Exception as e:
        print(f"Ошибка: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Использование: python3 wav2c.py sound_name")
        sys.exit(1)
    
    wav_to_progmem(sys.argv[1])
