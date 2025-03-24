with open("secret25.txt", "r") as f:
    print(len(f.read()))

with open("extracted25.txt", "r") as f:
    text = f.read()
    if chr(0) in text:
        print(len(text[:text.index(chr(0))]))
    else:
        print(len(text))