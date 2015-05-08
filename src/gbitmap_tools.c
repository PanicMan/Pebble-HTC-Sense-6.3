#include <pebble.h>
#include "gbitmap_tools.h"

/*
* Algorithm from http://www.compuphase.com/graphic/scale.htm and adapted for Pebble GBitmap
*/

static void scaleRow(uint8_t *target, uint8_t *source, int srcWidth, int tgtWidth, int srcOrigX)
{
	  int tgtPixels = 0;
	  int intPart = srcWidth / tgtWidth;
	  int fractPart = srcWidth % tgtWidth;
	  int E = 0;
	  int srcIndex = srcOrigX % 8;

	  source += srcOrigX / 8;

	  while (tgtPixels < tgtWidth) 
	  {
			*target  |= ((*source >> srcIndex) & 1) << (tgtPixels % 8);
			srcIndex += intPart;

			E += fractPart;
			if (E >= tgtWidth) 
			{
				E -= tgtWidth;
				srcIndex++;
			} 

			if(srcIndex >= 8)
			{
				source += srcIndex / 8;
				srcIndex = srcIndex % 8;
			}

			tgtPixels++;
			if(tgtPixels % 8 == 0)
				target++;
	  } 
}

GBitmap* scaleBitmapMono(GBitmap* src, uint8_t ratio_width_percent, uint8_t ratio_height_percent)
{
	GBitmap* tgt = NULL;
	if(ratio_width_percent <= 100 && ratio_height_percent <= 100)
	{
		int srcHeight = gbitmap_get_bounds(src).size.h;
		int srcWidth = gbitmap_get_bounds(src).size.w;
		int tgtHeight = srcHeight * ratio_height_percent / 100;
		int tgtWidth = srcWidth * ratio_width_percent / 100;

		tgt = gbitmap_create_blank_with_palette(GSize(tgtWidth, tgtHeight), gbitmap_get_format(src), gbitmap_get_palette(src), false);

		if(tgt == NULL)
			return NULL;

		if(tgtHeight != 0 && tgtWidth != 0)
		{
			uint16_t NumPixels = tgtHeight;
			uint16_t intPart = (srcHeight / tgtHeight) * gbitmap_get_bytes_per_row(src);
			uint16_t fractPart = srcHeight % tgtHeight;
			int E = 0;
			uint8_t *source = gbitmap_get_data(src) + gbitmap_get_bounds(src).origin.y * gbitmap_get_bytes_per_row(src);
			uint8_t *target = gbitmap_get_data(tgt);

			while (NumPixels-- > 0) 
			{
				scaleRow(target, source, srcWidth, tgtWidth, gbitmap_get_bounds(src).origin.x);
				target += gbitmap_get_bytes_per_row(tgt);
				source += intPart;
				E += fractPart;
				if (E >= tgtHeight) {
					E -= tgtHeight;
					source += gbitmap_get_bytes_per_row(src);
				} 
			} 
		}
	}

	return tgt;
}

// set pixel color at given coordinates 
void set_pixel(uint8_t *bitmap_data, uint16_t bytes_per_row, int y, int x, uint8_t color) 
//void set_pixel(GBitmap* tgt, int y, int x, uint8_t color) 
{
	#ifdef PBL_COLOR 
		//uint8_t *bitmap_data = gbitmap_get_data(tgt);
		//uint16_t bytes_per_row = gbitmap_get_bytes_per_row(tgt);
		bitmap_data[y*bytes_per_row + x] = color; // in Basalt - simple set entire byte
	#else
		//uint8_t *bitmap_data = (uint8_t)tgt->addr;
		//uint16_t bytes_per_row = tgt->row_size_bytes;
		bitmap_data[y*bytes_per_row + x / 8] ^= (-color ^ bitmap_data[y*bytes_per_row + x / 8]) & (1 << (x % 8)); // in Aplite - set the bit
	#endif
}

// get pixel color at given coordinates 
uint8_t get_pixel(uint8_t *bitmap_data, uint16_t bytes_per_row, int y, int x) 
//uint8_t get_pixel(GBitmap* src, int y, int x) 
{
	#ifdef PBL_COLOR
		//uint8_t *bitmap_data = gbitmap_get_data(src);
		//uint16_t bytes_per_row = gbitmap_get_bytes_per_row(src);
		return bitmap_data[y*bytes_per_row + x]; // in Basalt - simple get entire byte
	#else
		//uint8_t *bitmap_data = (uint8_t)src->addr;
		//uint16_t bytes_per_row = src->row_size_bytes;
		return (bitmap_data[y*bytes_per_row + x / 8] >> (x % 8)) & 1; // in Aplite - get the bit
	#endif
}

GBitmap* scaleBitmap(GBitmap* src, uint8_t ratio_width_percent, uint8_t ratio_height_percent)
{
	GBitmap* tgt = NULL;
	uint16_t srcHeight = gbitmap_get_bounds(src).size.h, srcWidth = gbitmap_get_bounds(src).size.w;
	uint16_t tgtHeight = srcHeight * ratio_height_percent / 100, tgtWidth = srcWidth * ratio_width_percent / 100;

	tgt = gbitmap_create_blank_with_palette(GSize(tgtWidth, tgtHeight), gbitmap_get_format(src), gbitmap_get_palette(src), false);
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "SizeSrc: %dx%d, SizeTgt: %dx%d, Format: %d/%d", 
			srcWidth, srcHeight, tgtWidth, tgtHeight, gbitmap_get_format(src), gbitmap_get_format(tgt));

	if(tgt != NULL && tgtHeight != 0 && tgtWidth != 0)
	{
		int x_ratio = (int)((srcWidth * 100) / tgtWidth);
		int y_ratio = (int)((srcHeight * 100) / tgtHeight);

		uint8_t *bmpDataSrc = gbitmap_get_data(src), *bmpDataTgt = gbitmap_get_data(tgt);
		uint16_t bprSrc = gbitmap_get_bytes_per_row(src), bprTgt = gbitmap_get_bytes_per_row(tgt);
		
		app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "x_ratio: %d, y_ratio: %d, bprSrc: %d, bprTgt: %d, bmpDataSrc: 0x%08X, bmpDataTgt: 0x%08X", 
				x_ratio, y_ratio, bprSrc, bprTgt, (unsigned int)bmpDataSrc, (unsigned int)bmpDataTgt);

		for (int y=0; y<tgtHeight; y++) 
			for (int x=0; x<tgtWidth; x++) 
			{
				int x2 = ((x*x_ratio) / 100), y2 = ((y*y_ratio) / 100);
				uint8_t color = get_pixel(bmpDataSrc, bprSrc, x2, y2);
				set_pixel(bmpDataTgt, bprTgt, x, y, color);
			}                
	}
	return tgt;
}

/*
#define setBlackPixel(bmp, x, y) ((((uint8_t *)gbitmap_get_data(bmp))[(y) * gbitmap_get_bytes_per_row(bmp) + (x) / 8] &= ~(0x01 << ((x)%8))))
#define setWhitePixel(bmp, x, y) ((((uint8_t *)gbitmap_get_data(bmp))[y * gbitmap_get_bytes_per_row(bmp) + x / 8] |= (0x01 << (x%8))))
#define getPixel(bmp, x, y)\
(((x) >= gbitmap_get_bounds(bmp).size.w || (y) >= gbitmap_get_bounds(bmp).size.h || (x) < 0 || (y) < 0) ?\
 -1 :\
 ((((uint8_t *)gbitmap_get_data(bmp))[(y)*gbitmap_get_bytes_per_row(bmp) + (x)/8] & (1<<((x)%8))) != 0))


// findNearestPixel : algorithm adapted from Jnm code : https://github.com/Jnmattern/Minimalist_2.0/blob/master/src/bitmap.h#L336
static inline GPoint findNearestPixel(GBitmap* bmp, int px, int py, GColor color) {
  int maximum_radius_square = gbitmap_get_bounds(bmp).size.h * gbitmap_get_bounds(bmp).size.h + gbitmap_get_bounds(bmp).size.w * gbitmap_get_bounds(bmp).size.w;
  int radius = 1;

  while(radius * radius < maximum_radius_square){
    int x = 0, y = radius, d = radius-1;
    while (y >= x) {
      if (getPixel(bmp, px+x, py+y) == color) return GPoint(px+x, py+y);
      if (getPixel(bmp, px+y, py+x) == color) return GPoint(px+y, py+x);
      if (getPixel(bmp, px-x, py+y) == color) return GPoint(px-x, py+y);
      if (getPixel(bmp, px-y, py+x) == color) return GPoint(px-y, py+x);
      if (getPixel(bmp, px+x, py-y) == color) return GPoint(px+x, py-y);
      if (getPixel(bmp, px+y, py-x) == color) return GPoint(px+y, py-x);
      if (getPixel(bmp, px-x, py-y) == color) return GPoint(px-x, py-y);
      if (getPixel(bmp, px-y, py-x) == color) return GPoint(px-y, py-x);

      if (d >= 2*x-2) {
        d = d-2*x;
        x++;
      } else if (d <= 2*radius - 2*y) {
        d = d+2*y-1;
        y--;
      } else {
        d = d + 2*y - 2*x - 2;
        y--;
        x++;
      }
    }
    radius++;
  }
  return GPoint(0, 0);
}

#define signum(x) (x < 0 ? -1 : 1)

void computeMorphingBitmap(GBitmap* source, GBitmap* dest, GBitmap* result, uint8_t remainingSteps) {
  if(gbitmap_get_bytes_per_row(source) != gbitmap_get_bytes_per_row(dest) || 
	 gbitmap_get_bounds(source).size.w != gbitmap_get_bounds(dest).size.w || gbitmap_get_bounds(source).size.h != gbitmap_get_bounds(dest).size.h)
    return;

  memset(gbitmap_get_data(result), 0xFF, sizeof(uint8_t) * gbitmap_get_bytes_per_row(source) * gbitmap_get_bounds(source).size.h);

  GColor sourceColor, destColor;
  int diffX,diffY;

  int width   = gbitmap_get_bounds(source).size.w;
  int height  = gbitmap_get_bounds(source).size.h;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      sourceColor = getPixel(source,  x, y);
      destColor   = getPixel(dest,  x, y);
      if (sourceColor == GColorBlack && destColor == GColorBlack) {
        setBlackPixel(result, x, y);
      } else if (sourceColor == GColorBlack) {
        GPoint p = findNearestPixel(dest, x, y, GColorBlack);
        diffX = (p.x - x) / remainingSteps;
        diffY = (p.y - y) / remainingSteps;
        if(diffX == 0 && diffY == 0){
          if(abs(p.x - x) > abs(p.y - y)){
            diffX = signum(p.x - x);
          }
          else {
            diffY = signum(p.y - y);
          }
        }
        setBlackPixel(result, x + diffX, y + diffY);
      } else if (destColor == GColorBlack) {
        GPoint p = findNearestPixel(source, x, y, GColorBlack);
        diffX = (x - p.x) / remainingSteps;
        diffY = (y - p.y) / remainingSteps;
        if(diffX == 0 && diffY == 0){
          if(abs(p.x - x) > abs(p.y - y)){
            diffX = signum(x - p.x);
          }
          else {
            diffY = signum(y - p.y);
          }
        }
        setBlackPixel(result, p.x + diffX, p.y + diffY);
      }
    }
  }
}
*/