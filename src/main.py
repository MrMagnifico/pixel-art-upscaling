from pixel_io import read_png, SVGWriter
from kopf_lichinski import Vectorizer
from os import path


TEST_FILES = [
    # "SonictheHedgehog_SonicSprite", # Sonic breaks the current implementation
    "Sonic_screech",
    # "gaxe_skeleton_input", # So does the poor skeleton
]


if __name__ == "__main__":
    for filename in TEST_FILES:
        input_file = path.join("data", f"{filename}.png")

        height, width, image_data = read_png(input_file)
        pixel_data = Vectorizer(width, height, image_data)
        pixel_data.vectorize()
        writer = SVGWriter(pixel_data, filename)
        writer.output_image([2, 4, 8, 16], width, height)
