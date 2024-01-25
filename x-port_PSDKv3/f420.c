
void Bitmap2Yuv420p_calc2(uint8_t *stride1, uint8_t *stride2, uint8_t *stride3, uint8_t *rgb, size_t width, size_t height)
{
    size_t image_size = width * height;
    size_t upos = 0;
    size_t vpos = 0;
    size_t i = 0;
    uint8_t r,g,b;

    for( size_t line = 0; line < height; ++line )
    {

        if( !(line % 2) )
        {
            for( size_t x = 0; x < width; x += 2 )
            {
                r = rgb[3 * i];
                g = rgb[3 * i + 1];
                b = rgb[3 * i + 2];

                stride1[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
                stride2[upos++] = ((-38*r + -74*g + 112*b) >> 8) + 128;
                stride3[vpos++] = ((112*r + -94*g + -18*b) >> 8) + 128;

                r = rgb[3 * i];
                g = rgb[3 * i + 1];
                b = rgb[3 * i + 2];

                stride1[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
        else
        {
            for( size_t x = 0; x < width; x += 1 )
            {
                r = rgb[3 * i];
                g = rgb[3 * i + 1];
                b = rgb[3 * i + 2];
                stride1[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
    }
}