import pygame

# insert ':' between every 2 digits to show on display as MAC address
def convert_to_mac(val):
    res = ''
    for idx in range(0,len(val)):
        res += val[idx]
        if idx % 2 == 1 and idx != 0 and idx != len(val)-1:
            res += ':'

    return res

def drawText(surface, text, color, rect, font, aa=False, bkg=None):
    rect = pygame.Rect(rect)
    y = rect.top
    lineSpacing = -2

    # get the height of the font
    fontHeight = font.size("Tg")[1]

    while text:
        i = 1

        # determine if the row of text will be outside our area
        if y + fontHeight > rect.bottom:
            break

        # determine maximum width of line
        while font.size(text[:i])[0] < rect.width and i < len(text):
            i += 1

        # if we've wrapped the text, then adjust the wrap to the last word      
        if i < len(text): 
            i = text.rfind(" ", 0, i) + 1

        # render the line and blit it to the surface
        if bkg:
            image = font.render(text[:i], 1, color, bkg)
            image.set_colorkey(bkg)
        else:
            image = font.render(text[:i], aa, color)

        surface.blit(image, (rect.left, y))
        y += fontHeight + lineSpacing

        # remove the text we just blitted
        text = text[i:]

    return text

def wrapText(text, x, y, color, width, font, screen):
    lines = []
    current_line = ""
    
    for char in text:
        temp_line = current_line + char
        if font.size(temp_line)[0] <= width:
            current_line = temp_line
        else:
            lines.append(current_line)
            current_line = char
    
    lines.append(current_line)

    for line in lines:
        text_surface = font.render(line, True, color)
        screen.blit(text_surface, (x,y))
        y += font.get_height()