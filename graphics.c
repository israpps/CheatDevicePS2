#include "graphics.h"
#include "stb_font.h"
#include "libraries/upng.h"
#include <time.h>
#include <graph.h>
#include <stdio.h>

static struct menuIcon {
		char *label;
		GSTEXTURE *tex;
};

static GSGLOBAL *gsGlobal;
static GSTEXTURE bg;
static GSTEXTURE check;
static GSTEXTURE font;
static GSTEXTURE gamepad;
static GSTEXTURE cube;
static GSTEXTURE cogs;
static stb_fontchar fontdata[STB_SOMEFONT_NUM_CHARS];
static int initialized = 0;

extern u8  _background_png_start[];
extern int _background_png_size;
extern u8  _check_png_start[];
extern int _check_png_size;
extern u8  _gamepad_png_start[];
extern int _gamepad_png_size;
extern u8  _cube_png_start[];
extern int _cube_png_size;
extern u8  _cogs_png_start[];
extern int _cogs_png_size;

static void graphicsLoadPNG(GSTEXTURE *tex, u8 *data, int len, int linear_filtering);

static u64 graphicsColorTable[] = { GS_SETREG_RGBAQ(0x00,0x00,0x00,0xFF,0xFF), // BLACK
									GS_SETREG_RGBAQ(0xF0,0xF0,0xF0,0xFF,0x00), // WHITE
									GS_SETREG_RGBAQ(0xF0,0x00,0x00,0xFF,0xFF), // RED
									GS_SETREG_RGBAQ(0x00,0xF0,0x00,0xFF,0xFF), // GREEN
									GS_SETREG_RGBAQ(0x00,0x00,0xF0,0xFF,0xFF), // BLUE
									GS_SETREG_RGBAQ(0xF0,0xB0,0x00,0xFF,0x80) }; // YELLOW

int initGraphicsMan()
{
	if(!initialized)
	{
		printf("\n ** Initializing Graphics Manager **\n");
		dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
					D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

		// Initialize the DMAC
		dmaKit_chan_init(DMA_CHANNEL_GIF);

		// Initialize the GS
		gsGlobal = gsKit_init_global();
		gsGlobal->PrimAAEnable = GS_SETTING_OFF;
		gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
		gsGlobal->DoubleBuffering = GS_SETTING_OFF;
		gsGlobal->PSM = GS_PSM_CT32;
		gsKit_init_screen( gsGlobal );
		gsKit_mode_switch( gsGlobal, GS_ONESHOT );

		// Clear the screen right away to prevent the old framebuffer from being dumped to screen
		gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00));
		gsKit_sync_flip( gsGlobal );
		gsKit_queue_exec( gsGlobal );

		font.Width = STB_SOMEFONT_BITMAP_WIDTH;
		font.Height = STB_SOMEFONT_BITMAP_HEIGHT;
		font.PSM = GS_PSM_T8;
		font.ClutPSM = GS_PSM_CT32;
		font.Mem = memalign(128, gsKit_texture_size_ee(font.Width, font.Height, font.PSM));
		font.Clut = memalign(128, gsKit_texture_size_ee(16, 16, font.ClutPSM));
		font.VramClut = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(16, 16, font.ClutPSM), GSKIT_ALLOC_USERBUFFER);
		font.Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(font.Width, font.Height, font.PSM), GSKIT_ALLOC_USERBUFFER);
		font.Filter = GS_FILTER_NEAREST;

		/* Generate palette */
		unsigned int i;
		for(i = 0; i < 256; ++i)
		{
			u8 alpha = (i > 0x80) ? 0x80 : i;
			font.Clut[i] = i | (i << 8) | (i << 16) | (alpha << 24);
		}

		STB_SOMEFONT_CREATE(fontdata, font.Mem, STB_SOMEFONT_BITMAP_HEIGHT);
		gsKit_texture_upload(gsGlobal, &font);

		graphicsLoadPNG(&bg, _background_png_start, _background_png_size, 0);
		graphicsLoadPNG(&check, _check_png_start, _check_png_size, 0);
		graphicsLoadPNG(&gamepad, _gamepad_png_start, _gamepad_png_size, 0);
		graphicsLoadPNG(&cube, _cube_png_start, _cube_png_size, 0);
		graphicsLoadPNG(&cogs, _cogs_png_start, _cogs_png_size, 0);

		return 1;
	}
	else
		return 0;
}

static void graphicsLoadPNG(GSTEXTURE *tex, u8 *data, int len, int linear_filtering)
{
	printf("loading a png\n");
	upng_t* pngTexture = upng_new_from_bytes(data, len);
	upng_header(pngTexture);
	upng_decode(pngTexture);

	tex->Width = upng_get_width(pngTexture);
	tex->Height = upng_get_height(pngTexture);
	
	if(upng_get_format(pngTexture) == UPNG_RGB8)
		tex->PSM = GS_PSM_CT24;
	else if(upng_get_format(pngTexture) == UPNG_RGBA8)
		tex->PSM = GS_PSM_CT32;

	tex->Mem = memalign(128, gsKit_texture_size_ee(tex->Width, tex->Height, tex->PSM));
	memcpy(tex->Mem, upng_get_buffer(pngTexture), gsKit_texture_size_ee(tex->Width, tex->Height, tex->PSM));
	tex->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(tex->Width, tex->Height, tex->PSM), GSKIT_ALLOC_USERBUFFER);
	tex->Filter = (linear_filtering) ? GS_FILTER_LINEAR : GS_FILTER_NEAREST;
	gsKit_texture_upload(gsGlobal, tex);
}

static void graphicsPrintText(int x, int y, const char *txt, u64 color)
{
	char const *cptr = txt;
	int cx = x;
	int cy = y;

	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);

	while(*cptr)
	{
		if(*cptr == '\n')
		{
			cy += 22;
			cx = x;
			cptr++;
			continue;
		}
		int char_codepoint = *cptr++;
		stb_fontchar *cdata = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];

		///*
		gsKit_prim_quad_texture(gsGlobal, &font,	cx + cdata->x0f, cy + cdata->y0f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s0f, cdata->t0f*STB_SOMEFONT_BITMAP_HEIGHT,
													cx + cdata->x1f, cy + cdata->y0f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s1f, cdata->t0f*STB_SOMEFONT_BITMAP_HEIGHT,
													cx + cdata->x0f, cy + cdata->y1f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s0f, cdata->t1f*STB_SOMEFONT_BITMAP_HEIGHT,
													cx + cdata->x1f, cy + cdata->y1f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s1f, cdata->t1f*STB_SOMEFONT_BITMAP_HEIGHT,
													1, color);

		cx += cdata->advance_int;
	}

	gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
}

void graphicsDrawText(int x, int y, const char *txt, graphicsColor_t color)
{
	graphicsPrintText(x, y, txt, graphicsColorTable[color]);
}

void graphicsDrawLoadingBar(int x, int y, float progress)
{
	int height = 10;
	int width = gsGlobal->Width - 2*x;
	u64 color = GS_SETREG_RGBAQ(0x22, 0x22, 0xee, 0x25, 0x00);
	u64 outline = GS_SETREG_RGBAQ(0x00, 0x00, 0x00, 0x00, 0x00);

	if(progress < 0.0)
		progress = 0.0;
	if(progress > 1.0)
		progress = 1.0;

	// outline
	gsKit_prim_quad(gsGlobal, x-5, y-5,
							  x-5, y+height+5,
							  x+5+width, y-5,
							  x+5+width, y+height+5, 1, outline);

	// progress bar
	gsKit_prim_quad(gsGlobal, x, y,
							  x, y+height,
							  x + (progress * width), y,
							  x + (progress * width), y+height, 1, color);
}

void graphicsDrawPromptBox(int width, int height)
{
	const int x0 = (gsGlobal->Width/2) - (width/2);
	const int x1 = (gsGlobal->Width/2) + (width/2);
	const int y0 = (gsGlobal->Height/2) - (height/2);
	const int y1 = (gsGlobal->Height/2) + (height/2);
	const u64 color = GS_SETREG_RGBAQ(0x22, 0x22, 0xEE, 0x25, 0x00);

	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);

	gsKit_prim_quad(gsGlobal, x0, y0,
							  x1, y0,
							  x0, y1,
							  x1, y1, 1, color);

	gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
}

static void drawMenu(struct menuIcon icons[], int numIcons, int activeItem)
{
	const u64 unselected = GS_SETREG_RGBAQ(0xA0, 0xA0, 0xA0, 0x20, 0x80);
	const u64 selected = GS_SETREG_RGBAQ(0x50, 0x50, 0x50, 0x80, 0x80);
	
	graphicsDrawPromptBox(350, 150);
	
	int **dimensions = malloc(numIcons * 3 * sizeof(int));
	
	int i;
	for(i = 0; i < numIcons; i++)
	{
		int x = (gsGlobal->Width / 2) - ((75 * numIcons) / 2) + (75 * i);
		gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);
		gsKit_prim_sprite_texture(gsGlobal, icons[i].tex,
											x,
											192,
											0,
											0,
											x + (icons[i].tex)->Width,
											192 + (icons[i].tex)->Height,
											(icons[i].tex)->Width,
											(icons[i].tex)->Height,
											1,
											(activeItem == i) ? selected : unselected);
		if (activeItem == i) graphicsDrawText(200, 250, icons[i].label, WHITE);
		gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
	}
}

void graphicsDrawMainMenu(int activeItem)
{
	struct menuIcon icons[] = {{"Start Game", &gamepad},
							   {"Game List", &cube},
							   {"Settings", &cogs}};
	
	drawMenu(icons, 3, activeItem);
}

void graphicsClearScreen(int r, int g, int b)
{
	gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(r, g, b,0x00,0x00));
}

void graphicsDrawBackground()
{
	gsKit_prim_sprite_texture(gsGlobal, &bg,
										0,							  // X1
										0,							  // Y1
										0,							  // U1
										0,							  // V1
										bg.Width,					  // X2
										bg.Height, 					  // Y2
										bg.Width, 					  // U2
										bg.Height,		 			  // V2
										1,							  // Z
										0x80808080					  // RGBAQ
										);
}

void graphicsDrawPointer(int x, int y)
{
	gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);
	gsKit_prim_sprite_texture(gsGlobal, &check,
										x,
										y,
										0,
										0,
										x + check.Width,
										y + check.Height,
										check.Width,
										check.Height,
										1,
										0x80808080);
	gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
}

void graphicsDrawAboutPage()
{
	static int x = 20;
	static int y = 100;
	static int v_x = 2;
	static int v_y = 2;

	if(x == 420) // right
		v_x = -2;
	else if(x == 20) // left
		v_x = 2;
	if(y == 310) // bottom
		v_y = -2;
	else if(y == 80) // top
		v_y = 2;

	x += v_x;
	y += v_y;
	
	graphicsDrawText(x, y, "Cheat Device\nBy Wesley Castro\nhttp://wescastro.com", WHITE);
	
	static int ticker_x = 0;
	if (ticker_x < 1100)
		ticker_x+= 2;
	else
		ticker_x = 0;
	graphicsDrawText(640 - ticker_x, 405, "Press CIRCLE to return to the game list.", WHITE);
}

void graphicsRenderNow()
{
	gsKit_sync_flip( gsGlobal );
	gsKit_queue_exec( gsGlobal );
}

void graphicsRender()
{
	static clock_t c = 0;

	if(clock() <= (c + 9600)) // (c + CLOCKS_PER_SEC/60)
		c = clock();

	else
	{
		gsKit_sync_flip( gsGlobal );
		gsKit_queue_exec( gsGlobal );
	}
}
