
import os
import re
import importlib.util

os.chdir(os.path.dirname(__file__))

def modExists(modName):
    spec = importlib.util.find_spec(modName)
    return spec is not None

if not modExists("PIL"):
    print("PLEASE INSTALL PIL FOR PYTHON")
    print("pip install pillow")
    exit(1)

from PIL import Image
import colorsys

def invertHSL(colors):
 
    res = []
    
    for color in colors:

        r, g, b = (color & 0xFF) >> 0, (color & 0xFF00) >> 8, (color & 0xFF0000) >> 16
        r /= 255
        g /= 255
        b /= 255

        h, l, s = colorsys.rgb_to_hls(r, g, b)
    
        h = (h - 0.1888)
        if h < 0:
            h += 1
        h = h % 1
    
        r, g, b = colorsys.hls_to_rgb(h, l, s)

        r *= 255
        g *= 255
        b *= 255

        res.append( (color & 0xFF000000) | ((int(r) << 0) | (int(g) << 8) | (int(b) << 16)) )

    return res

files = [ f for f in os.listdir("./") ]
files.sort()

data = {}

for file in files:

    pattern = r"(\d+)\_(\d+)\.png" # best to not upload these. copyright
    match = re.match(pattern, file)

    if match:
        char, pal = int(match.group(1)), int(match.group(2))

        image = Image.open(f"{char}_{pal}.png")

        if char not in data:
            data[char] = {}

        if image.mode == "P":
            palette = image.getpalette()

            colors = [ 0x01000000 | ((palette[i] << 0) | (palette[i + 1] << 8) | (palette[i + 2] << 16)) for i in range(0, len(palette), 3)]
            
            if char == 12 and pal == 40:
                colors = invertHSL(colors)

            data[char][pal] = [ f"0x{c:08X}" for c in colors ]

        else:
            print(f"{file} was not an indexed png")

        


output = []

output.append('#include "palettes.hpp"')

output.append("std::map<int, std::map<int, std::array<DWORD, 256>>> palettes = {")

for char in sorted(data.keys()):
    output.append(f"{{{char},{{")
    for pal in sorted(data[char].keys()):
        paletteData = ",".join(data[char][pal])
        output.append(f"{{{pal},{{{paletteData}}}}},")
    output.append(f"}}}},")    
        

output.append("};")

with open("palettes.cpp", "w+") as f:
    for line in output:
        f.write(line + "\n")
 