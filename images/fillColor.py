import glob

for file in glob.glob("*.svg"):
    with open(file, "r", encoding="utf-8") as f:
        data = f.read()
    data = data.replace('fill="currentColor"', 'fill="white"')
    data = data.replace('stroke="currentColor"', 'stroke="white"')
    with open(file, "w", encoding="utf-8") as f:
        f.write(data)
