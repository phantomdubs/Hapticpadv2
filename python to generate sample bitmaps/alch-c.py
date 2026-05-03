import os
from PIL import Image, ImageDraw, ImageFont

# 1. Define the 6 Profiles and their 6 Icon Labels
icon_data = {
    "StarNavC": ["WRP", "MAP", "SCN", "COM", "SHD", "FIR"],
    "Mech-T":   ["L-L", "TRS", "L-R", "MSL", "LSR", "EJT"],
    "CybMix-M": ["LUP", "WAV", "DRP", "BAS", "MID", "HGH"],
    "Alch-C":   ["HOT", "COL", "ADD", "STR", "PUR", "DN!"],
    "Hack-T":   ["BYP", "INJ", "CRK", "GHS", "TRC", "NUK"],
    "Race-M":   ["NIT", "BRK", "DFT", "CAM", "LGT", "RST"]
}

def generate_bitmaps():
    # 2. Iterate through folders
    for folder, labels in icon_data.items():
        if not os.path.exists(folder):
            os.makedirs(folder)
            print(f"Created folder: {folder}")

        for i, label in enumerate(labels):
            # 3. Create a 24x24 pixel image in 1-bit mode ('1')
            # Mode '1' ensures the 1-bit depth required by your C++ code 
            img = Image.new('1', (24, 24), 0) # 0 = Black background
            draw = ImageDraw.Draw(img)

            # 4. Draw a simple white border
            draw.rectangle([0, 0, 23, 23], outline=1)

            # 5. Draw the Label text
            # Note: Default font is very small; this fits 3 letters perfectly
            try:
                draw.text((3, 7), label, fill=1)
            except Exception:
                # Fallback if text fails
                draw.line([6,6, 18,18], fill=1, width=2)

            # 6. Save as BMP
            file_path = os.path.join(folder, f"{i+1}.bmp")
            img.save(file_path, "BMP")
            
    print("\nSuccess! 36 icons generated across 6 folders.")

if __name__ == "__main__":
    generate_bitmaps()