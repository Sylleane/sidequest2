/**
 * stb_image - v2.28 - public domain image loader
 * http://nothings.org/stb
 * 
 * Ce fichier est une version simplifiée de stb_image pour charger
 * les images PNG, JPG et GIF.
 * 
 * Voir: https://github.com/nothings/stb
 */

#ifndef STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG  
#define STBI_ONLY_GIF
#endif

// Version minimale inline pour notre usage - télécharge l'original pour production
// Cette version simplifiée gère juste le minimum pour les GIFs

#ifndef STBI_INCLUDE_STB_IMAGE_H
#define STBI_INCLUDE_STB_IMAGE_H

#include <stdlib.h>
#include <string.h>

typedef unsigned char stbi_uc;
typedef unsigned short stbi_us;

#ifdef __cplusplus
extern "C" {
#endif

// Fonctions de base
stbi_uc* stbi_load_from_memory(stbi_uc const* buffer, int len, int* x, int* y, int* comp, int req_comp);
stbi_uc* stbi_load_gif_from_memory(stbi_uc const* buffer, int len, int** delays, int* x, int* y, int* z, int* comp, int req_comp);
void stbi_image_free(void* retval_from_stbi_load);
const char* stbi_failure_reason(void);

#ifdef __cplusplus
}
#endif

#endif // STBI_INCLUDE_STB_IMAGE_H

#ifdef STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif

typedef unsigned char stbi__uint8;
typedef unsigned short stbi__uint16;
typedef signed short stbi__int16;
typedef unsigned int stbi__uint32;
typedef signed int stbi__int32;

static char* stbi__g_failure_reason = nullptr;

const char* stbi_failure_reason(void)
{
    return stbi__g_failure_reason;
}

static int stbi__err(const char* str)
{
    stbi__g_failure_reason = (char*)str;
    return 0;
}

void stbi_image_free(void* retval_from_stbi_load)
{
    free(retval_from_stbi_load);
}

// Contexte de lecture
typedef struct
{
    stbi__uint8 const* img_buffer;
    stbi__uint8 const* img_buffer_end;
    stbi__uint8 const* img_buffer_original;
} stbi__context;

static void stbi__refill_buffer(stbi__context* s) {}

static stbi__uint8 stbi__get8(stbi__context* s)
{
    if (s->img_buffer < s->img_buffer_end)
        return *s->img_buffer++;
    return 0;
}

static int stbi__at_eof(stbi__context* s)
{
    return s->img_buffer >= s->img_buffer_end;
}

static void stbi__skip(stbi__context* s, int n)
{
    if (n < 0)
    {
        s->img_buffer = s->img_buffer_end;
        return;
    }
    s->img_buffer += n;
}

static int stbi__getn(stbi__context* s, stbi__uint8* buffer, int n)
{
    if (s->img_buffer + n <= s->img_buffer_end)
    {
        memcpy(buffer, s->img_buffer, n);
        s->img_buffer += n;
        return 1;
    }
    return 0;
}

static int stbi__get16le(stbi__context* s)
{
    int z = stbi__get8(s);
    return z + (stbi__get8(s) << 8);
}

static stbi__uint32 stbi__get32le(stbi__context* s)
{
    stbi__uint32 z = stbi__get16le(s);
    return z + (stbi__get16le(s) << 16);
}

// ============================================================
// GIF loader
// ============================================================

typedef struct
{
    stbi__int16 prefix;
    stbi__uint8 first;
    stbi__uint8 suffix;
} stbi__gif_lzw;

typedef struct
{
    int w, h;
    stbi__uint8* out;
    stbi__uint8* background;
    stbi__uint8* history;
    int flags, bgindex, ratio, transparent, eflags;
    stbi__uint8 pal[256][4];
    stbi__uint8 lpal[256][4];
    stbi__gif_lzw codes[8192];
    stbi__uint8* color_table;
    int parse, step;
    int lflags;
    int start_x, start_y;
    int max_x, max_y;
    int cur_x, cur_y;
    int line_size;
    int delay;
} stbi__gif;

static int stbi__gif_test_raw(stbi__context* s)
{
    int sz;
    if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8')
        return 0;
    sz = stbi__get8(s);
    if (sz != '9' && sz != '7')
        return 0;
    if (stbi__get8(s) != 'a')
        return 0;
    return 1;
}

static int stbi__gif_test(stbi__context* s)
{
    int r = stbi__gif_test_raw(s);
    s->img_buffer = s->img_buffer_original;
    return r;
}

static void stbi__gif_parse_colortable(stbi__context* s, stbi__uint8 pal[][4], int num_entries, int transp)
{
    for (int i = 0; i < num_entries; ++i)
    {
        pal[i][2] = stbi__get8(s);
        pal[i][1] = stbi__get8(s);
        pal[i][0] = stbi__get8(s);
        pal[i][3] = transp == i ? 0 : 255;
    }
}

static int stbi__gif_header(stbi__context* s, stbi__gif* g, int* comp, int is_info)
{
    stbi__uint8 version;
    if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8')
        return stbi__err("not GIF");
    version = stbi__get8(s);
    if (version != '7' && version != '9')
        return stbi__err("not GIF");
    if (stbi__get8(s) != 'a')
        return stbi__err("not GIF");

    g->w = stbi__get16le(s);
    g->h = stbi__get16le(s);
    g->flags = stbi__get8(s);
    g->bgindex = stbi__get8(s);
    g->ratio = stbi__get8(s);
    g->transparent = -1;

    if (comp != 0)
        *comp = 4;

    if (is_info)
        return 1;

    if (g->flags & 0x80)
        stbi__gif_parse_colortable(s, g->pal, 2 << (g->flags & 7), -1);

    return 1;
}

static int stbi__gif_info_raw(stbi__context* s, int* x, int* y, int* comp)
{
    stbi__gif* g = (stbi__gif*)malloc(sizeof(stbi__gif));
    if (!stbi__gif_header(s, g, comp, 1))
    {
        free(g);
        s->img_buffer = s->img_buffer_original;
        return 0;
    }
    if (x)
        *x = g->w;
    if (y)
        *y = g->h;
    free(g);
    return 1;
}

static void stbi__out_gif_code(stbi__gif* g, stbi__uint16 code)
{
    stbi__uint8* p;
    stbi__uint8* c;
    int idx;

    if (g->codes[code].prefix >= 0)
        stbi__out_gif_code(g, g->codes[code].prefix);

    if (g->cur_y >= g->max_y)
        return;

    idx = g->cur_x + g->cur_y;
    p = &g->out[idx];
    g->history[idx / 4] = 1;

    c = &g->color_table[g->codes[code].suffix * 4];
    if (c[3] > 128)
    {
        p[0] = c[2];
        p[1] = c[1];
        p[2] = c[0];
        p[3] = c[3];
    }
    g->cur_x += 4;

    if (g->cur_x >= g->max_x)
    {
        g->cur_x = g->start_x;
        g->cur_y += g->step;

        while (g->cur_y >= g->max_y && g->parse > 0)
        {
            g->step = (1 << g->parse) * g->line_size;
            g->cur_y = g->start_y + (g->step >> 1);
            --g->parse;
        }
    }
}

static stbi__uint8* stbi__process_gif_raster(stbi__context* s, stbi__gif* g)
{
    stbi__uint8 lzw_cs;
    stbi__int32 len, init_code;
    stbi__uint32 first;
    stbi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear, end;

    lzw_cs = stbi__get8(s);
    if (lzw_cs > 12)
        return NULL;
    clear = 1 << lzw_cs;
    first = 1;
    codesize = lzw_cs + 1;
    codemask = (1 << codesize) - 1;
    bits = 0;
    valid_bits = 0;
    for (init_code = 0; init_code < clear; init_code++)
    {
        g->codes[init_code].prefix = -1;
        g->codes[init_code].first = (stbi__uint8)init_code;
        g->codes[init_code].suffix = (stbi__uint8)init_code;
    }

    avail = clear + 2;
    oldcode = -1;
    end = clear + 1;

    len = 0;
    for (;;)
    {
        if (valid_bits < codesize)
        {
            if (len == 0)
            {
                len = stbi__get8(s);
                if (len == 0)
                    return g->out;
            }
            --len;
            bits |= (stbi__int32)stbi__get8(s) << valid_bits;
            valid_bits += 8;
        }
        else
        {
            stbi__int32 code = bits & codemask;
            bits >>= codesize;
            valid_bits -= codesize;

            if (code == clear)
            {
                codesize = lzw_cs + 1;
                codemask = (1 << codesize) - 1;
                avail = clear + 2;
                oldcode = -1;
                first = 0;
            }
            else if (code == end)
            {
                while ((len = stbi__get8(s)) > 0)
                    stbi__skip(s, len);
                return g->out;
            }
            else if (code <= avail)
            {
                if (first)
                {
                    return (stbi__uint8*)(size_t)stbi__err("no clear code");
                }

                if (oldcode >= 0)
                {
                    stbi__gif_lzw* p = &g->codes[avail++];
                    if (avail > 8192)
                    {
                        return (stbi__uint8*)(size_t)stbi__err("too many codes");
                    }

                    p->prefix = (stbi__int16)oldcode;
                    p->first = g->codes[oldcode].first;
                    p->suffix = (code == avail) ? p->first : g->codes[code].first;
                }
                else if (code == avail)
                    return (stbi__uint8*)(size_t)stbi__err("illegal code in raster");

                stbi__out_gif_code(g, (stbi__uint16)code);

                if ((avail & codemask) == 0 && avail <= 0x0FFF)
                {
                    codesize++;
                    codemask = (1 << codesize) - 1;
                }

                oldcode = code;
            }
            else
            {
                return (stbi__uint8*)(size_t)stbi__err("illegal code in raster");
            }
        }
    }
}

static stbi__uint8* stbi__gif_load_next(stbi__context* s, stbi__gif* g, int* comp, int req_comp, stbi__uint8* two_back)
{
    int dispose;
    int first_frame;
    int pi;
    int pcount;
    (void)req_comp;

    first_frame = 0;
    if (g->out == 0)
    {
        if (!stbi__gif_header(s, g, comp, 0))
            return 0;
        if (!g->w || !g->h)
            return (stbi__uint8*)(size_t)stbi__err("bad dimensions");
        g->out = (stbi__uint8*)malloc(4 * g->w * g->h);
        g->background = (stbi__uint8*)malloc(4 * g->w * g->h);
        g->history = (stbi__uint8*)malloc(g->w * g->h);
        if (!g->out || !g->background || !g->history)
            return (stbi__uint8*)(size_t)stbi__err("outofmem");

        memset(g->out, 0, 4 * g->w * g->h);
        memset(g->background, 0, 4 * g->w * g->h);
        memset(g->history, 0, g->w * g->h);
        first_frame = 1;
    }
    else
    {
        dispose = (g->eflags & 0x1C) >> 2;
        pcount = g->w * g->h;

        if ((dispose == 3) && (two_back == 0))
        {
            dispose = 2;
        }

        if (dispose == 3)
        {
            for (pi = 0; pi < pcount; ++pi)
            {
                if (g->history[pi])
                {
                    memcpy(&g->out[pi * 4], &two_back[pi * 4], 4);
                }
            }
        }
        else if (dispose == 2)
        {
            for (pi = 0; pi < pcount; ++pi)
            {
                if (g->history[pi])
                {
                    memcpy(&g->out[pi * 4], &g->background[pi * 4], 4);
                }
            }
        }

        memcpy(g->background, g->out, 4 * g->w * g->h);
    }

    memset(g->history, 0, g->w * g->h);

    for (;;)
    {
        int tag = stbi__get8(s);
        switch (tag)
        {
        case 0x2C: // Image Descriptor
        {
            stbi__int32 x, y, w, h;

            x = stbi__get16le(s);
            y = stbi__get16le(s);
            w = stbi__get16le(s);
            h = stbi__get16le(s);
            if (((x + w) > (g->w)) || ((y + h) > (g->h)))
                return (stbi__uint8*)(size_t)stbi__err("bad Image Descriptor");

            g->line_size = g->w * 4;
            g->start_x = x * 4;
            g->start_y = y * g->line_size;
            g->max_x = g->start_x + w * 4;
            g->max_y = g->start_y + h * g->line_size;
            g->cur_x = g->start_x;
            g->cur_y = g->start_y;

            if (w == 0)
                g->cur_y = g->max_y;

            g->lflags = stbi__get8(s);

            if (g->lflags & 0x40)
            {
                g->step = 8 * g->line_size;
                g->parse = 3;
            }
            else
            {
                g->step = g->line_size;
                g->parse = 0;
            }

            if (g->lflags & 0x80)
            {
                stbi__gif_parse_colortable(s, g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
                g->color_table = (stbi__uint8*)g->lpal;
            }
            else if (g->flags & 0x80)
            {
                g->color_table = (stbi__uint8*)g->pal;
            }
            else
                return (stbi__uint8*)(size_t)stbi__err("missing color table");

            if (!stbi__process_gif_raster(s, g))
                return NULL;

            return g->out;
        }

        case 0x21: // Extension
        {
            int block;
            int len;
            block = stbi__get8(s);
            if (block == 0xF9) // Graphic Control Extension
            {
                len = stbi__get8(s);
                if (len == 4)
                {
                    g->eflags = stbi__get8(s);
                    g->delay = 10 * stbi__get16le(s);
                    if (g->transparent >= 0)
                    {
                        g->pal[g->transparent][3] = 255;
                    }
                    if (g->eflags & 0x01)
                    {
                        g->transparent = stbi__get8(s);
                        if (g->transparent >= 0)
                        {
                            g->pal[g->transparent][3] = 0;
                        }
                    }
                    else
                    {
                        stbi__skip(s, 1);
                        g->transparent = -1;
                    }
                }
                else
                {
                    stbi__skip(s, len);
                    break;
                }
            }
            while ((len = stbi__get8(s)) != 0)
            {
                stbi__skip(s, len);
            }
            break;
        }

        case 0x3B: // GIF Terminator
            return (stbi__uint8*)(size_t)1;

        default:
            return (stbi__uint8*)(size_t)stbi__err("unknown code");
        }
    }
}

stbi_uc* stbi_load_gif_from_memory(stbi_uc const* buffer, int len, int** delays, int* x, int* y, int* z, int* comp, int req_comp)
{
    stbi__context s;
    s.img_buffer = buffer;
    s.img_buffer_end = buffer + len;
    s.img_buffer_original = buffer;

    stbi_uc* result = nullptr;
    stbi__gif g;
    memset(&g, 0, sizeof(g));

    int stride;
    stbi__uint8* u = nullptr;
    stbi__uint8* two_back = nullptr;
    stbi__uint8* out = nullptr;
    int layers = 0;
    int* all_delays = nullptr;

    u = stbi__gif_load_next(&s, &g, comp, req_comp, two_back);
    if (u == (stbi__uint8*)1)
        u = nullptr;
    if (!u)
    {
        if (out)
            free(out);
        if (g.out)
            free(g.out);
        if (g.background)
            free(g.background);
        if (g.history)
            free(g.history);
        return nullptr;
    }

    stride = g.w * g.h * 4;

    do
    {
        if (out)
        {
            void* tmp = realloc(out, layers * stride + stride);
            if (tmp == NULL)
            {
                free(out);
                if (g.out)
                    free(g.out);
                if (g.background)
                    free(g.background);
                if (g.history)
                    free(g.history);
                return (stbi_uc*)(size_t)stbi__err("outofmem");
            }
            out = (stbi__uint8*)tmp;
            void* tmp_d = realloc(all_delays, sizeof(int) * (layers + 1));
            if (tmp_d == NULL)
            {
                free(out);
                free(all_delays);
                if (g.out)
                    free(g.out);
                if (g.background)
                    free(g.background);
                if (g.history)
                    free(g.history);
                return (stbi_uc*)(size_t)stbi__err("outofmem");
            }
            all_delays = (int*)tmp_d;
        }
        else
        {
            out = (stbi__uint8*)malloc(stride);
            all_delays = (int*)malloc(sizeof(int));
            if (!out || !all_delays)
            {
                if (g.out)
                    free(g.out);
                if (g.background)
                    free(g.background);
                if (g.history)
                    free(g.history);
                return (stbi_uc*)(size_t)stbi__err("outofmem");
            }
        }
        memcpy(out + (layers * stride), u, stride);
        all_delays[layers] = g.delay ? g.delay : 100;
        ++layers;

        if (two_back)
            free(two_back);
        two_back = g.out;
        g.out = nullptr;

        u = stbi__gif_load_next(&s, &g, comp, req_comp, two_back);
        if (u == (stbi__uint8*)1)
            u = nullptr;
    } while (u);

    if (two_back)
        free(two_back);
    if (g.out)
        free(g.out);
    if (g.background)
        free(g.background);
    if (g.history)
        free(g.history);

    if (comp)
        *comp = 4;
    *x = g.w;
    *y = g.h;
    *z = layers;
    if (delays)
        *delays = all_delays;
    else
        free(all_delays);

    return out;
}

stbi_uc* stbi_load_from_memory(stbi_uc const* buffer, int len, int* x, int* y, int* comp, int req_comp)
{
    // Juste support GIF pour l'instant
    return nullptr;
}

#endif // STB_IMAGE_IMPLEMENTATION
