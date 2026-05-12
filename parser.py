import re
import sys

# Rozšířené mapování HID kódů
HID_MAP = {
    0x04: 'a', 0x05: 'b', 0x06: 'c', 0x07: 'd', 0x08: 'e', 0x09: 'f', 0x0A: 'g', 0x0B: 'h',
    0x0C: 'i', 0x0D: 'j', 0x0E: 'k', 0x0F: 'l', 0x10: 'm', 0x11: 'n', 0x12: 'o', 0x13: 'p',
    0x14: 'q', 0x15: 'r', 0x16: 's', 0x17: 't', 0x18: 'u', 0x19: 'v', 0x1A: 'w', 0x1B: 'x',
    0x1C: 'y', 0x1D: 'z', 0x1E: '1', 0x1F: '2', 0x20: '3', 0x21: '4', 0x22: '5', 0x23: '6',
    0x24: '7', 0x25: '8', 0x26: '9', 0x27: '0', 0x28: 'Enter', 0x29: 'Esc', 0x2A: 'Backspace',
    0x2B: 'Tab', 0x2C: 'Space', 0x2D: '-', 0x2E: '=', 0x2F: '[', 0x30: ']', 0x31: '\\',
    0x33: ';', 0x34: "'", 0x35: '`', 0x36: ',', 0x37: '.', 0x38: '/',
    0x3A: 'F1', 0x3B: 'F2', 0x3C: 'F3', 0x3D: 'F4', 0x3E: 'F5', 0x3F: 'F6', 0x40: 'F7',
    0x41: 'F8', 0x42: 'F9', 0x43: 'F10', 0x44: 'F11', 0x45: 'F12',
    0x4F: 'Right', 0x50: 'Left', 0x51: 'Down', 0x52: 'Up'
}

MODIFIERS = [
    (0x01, "Ctrl"), (0x02, "Shift"), (0x04, "Alt"), (0x08, "Win"),
    (0x10, "Ctrl"), (0x20, "Shift"), (0x40, "Alt"), (0x80, "Win"),
]

def main():
    if len(sys.argv) < 2:
        print("Použití: python parser.py log.txt")
        return

    # Regex pro: [timestamp] MOD:XX,KEYS:XX.XX...
    line_pattern = re.compile(r"\[(\d+)\] MOD:([0-9A-F]+),KEYS:([0-9A-F.]+)")

    try:
        with open(sys.argv[1], 'r') as f:
            for line in f:
                match = line_pattern.search(line)
                if not match:
                    continue

                # 1. Čas v sekundách
                ms_timestamp = int(match.group(1))
                sec_timestamp = ms_timestamp / 1000.0
                
                # 2. Modifikátory
                mod_val = int(match.group(2), 16)
                active_mods = []
                for bit, name in MODIFIERS:
                    if mod_val & bit and name not in active_mods:
                        active_mods.append(name)
                
                # 3. Klávesy
                keys_raw = match.group(3).split('.')
                active_keys = []
                for k in keys_raw:
                    if not k: continue
                    k_int = int(k, 16)
                    if k_int != 0:
                        # Pokud klávesu neznáme, vypíšeme původní hex kód ze souboru
                        active_keys.append(HID_MAP.get(k_int, f"<{k}>"))
                
                # Sestavení řádku
                combo = "+".join(active_mods + active_keys)
                
                # Vypíšeme jen řádky, kde se něco děje (MOD nebo KEYS)
                if combo:
                    print(f"[{sec_timestamp:0.3f}s] {combo}")

    except FileNotFoundError:
        print("Soubor nenalezen.")

if __name__ == "__main__":
    main()