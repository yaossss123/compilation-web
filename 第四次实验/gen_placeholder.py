from PIL import Image, ImageDraw
img = Image.new('RGB', (800, 200), 'white')
d = ImageDraw.Draw(img)
d.text((200, 80), 'flex/bison screenshot', fill='gray')
import os
path = os.path.join('c:', os.sep, 'Users', 'corey', 'Desktop',
                    '\u7f16\u8bd1\u5b9e\u9a8c', '\u7b2c\u56db\u6b21\u5b9e\u9a8c',
                    'image', '3.png')
img.save(path)
print(f'Saved to {path}')
