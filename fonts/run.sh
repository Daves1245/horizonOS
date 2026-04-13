# use fontbm to generate xml font files for bmfont2c.py
./fontbm --font-file ~/horizon/fonts/MonaspaceArgonNF-Light.otf --output monaspace-argon-light --data-format xml --font-size 16

# bmfont2c.py (have to setup bmfont2c.cfg as well - use [Font2], [Font3], ...[FontN])
 uv run python3 bmfont2c.py

# copy over files into kernel
cp fontlibrary.* ../kernel/kernel/fonts/
