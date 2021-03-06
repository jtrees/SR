/**
 *
 *  Copyright (C) 2016-2018 Roman Pauer
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is furnished to do
 *  so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef USE_SDL2
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_thread.h>
    #include <SDL2/SDL_version.h>
    #include <SDL2/SDL_mixer.h>
#else
    #include <SDL/SDL.h>
    #include <SDL/SDL_thread.h>
    #include <SDL/SDL_version.h>
    #include <SDL/SDL_mixer.h>
#endif
#include "Game_defs.h"

#define DEFINE_VARIABLES
    #include "Game_vars.h"
#undef DEFINE_VARIABLES

#include "main.h"
#include "Albion-proc.h"
#include "Albion-proc-vfs.h"
#include "Albion-sound.h"
#include "Albion-timer.h"
#include "Albion-music-midiplugin.h"
#include "Albion-music-midiplugin2.h"
#include "Game_config.h"
#include "Game_thread.h"
#include "virtualfs.h"
#include "display.h"
#include "audio.h"
#include "input.h"

extern char main_;
static void Game_Display_Create(void)
{
#ifdef USE_SDL2
    if (Display_Fullscreen)
    {
        Game_Window = SDL_CreateWindow("SDL Albion", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_INPUT_GRABBED);
    }
    else
    {
        Game_Window = SDL_CreateWindow("SDL Albion", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Display_Width, Display_Height, (Display_MouseLocked)?SDL_WINDOW_INPUT_GRABBED:0);
    }

    if (Game_Window != NULL)
    {
        Game_Renderer = SDL_CreateRenderer(Game_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (Game_Renderer != NULL)
        {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

            if (Display_Fullscreen)
            {
                SDL_RenderSetLogicalSize(Game_Renderer, Display_Width, Display_Height);
            }
        }
        else
        {
            SDL_DestroyWindow(Game_Window);
            Game_Window = NULL;
        }
    }

    if (Game_Window != NULL)
    {
        Game_Texture = SDL_CreateTexture(Game_Renderer, (Display_Bitsperpixel == 32)?SDL_PIXELFORMAT_ARGB8888:SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, Render_Width, Render_Height);
        if (Game_Texture == NULL)
        {
            SDL_DestroyRenderer(Game_Renderer);
            Game_Renderer = NULL;
            SDL_DestroyWindow(Game_Window);
            Game_Window = NULL;
        }
    }

    if (Game_Window != NULL)
    {
        int mousex, mousey;

        SDL_ShowCursor(SDL_DISABLE);

        if (Display_MouseLocked)
        {
            Game_OldCursor = SDL_GetCursor();
            SDL_SetCursor(Game_NoCursor);

            SDL_SetRelativeMouseMode(SDL_TRUE);

            SDL_WarpMouseInWindow(Game_Window, mouse_pos[1], mouse_pos[0]);
        }
        else
        {
            if (Display_Fullscreen)
            {
                Game_OldCursor = SDL_GetCursor();
                SDL_SetCursor(Game_NoCursor);

                SDL_WarpMouseInWindow(Game_Window, mouse_pos[1], mouse_pos[0]);
            }
            else
            {
                if (Game_MouseCursor == 2)
                {
                    Game_OldCursor = SDL_GetCursor();
                    SDL_SetCursor(Game_NoCursor);
                }
                else if (Game_MouseCursor == 1)
                {
                    Game_OldCursor = SDL_GetCursor();
                    SDL_SetCursor(Game_MinCursor);
                }

                SDL_GetMouseState(&mousex, &mousey);
                SDL_WarpMouseInWindow(Game_Window, mousex, mousey);
            }

            SDL_ShowCursor(SDL_ENABLE);
        }
    }
#else
    {
        Uint32 flags;

    #ifdef ALLOW_OPENGL
        if (Game_UseOpenGL)
        {
            if (Display_Bitsperpixel == 32)
            {
                SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
                SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
                SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
            }
            else if (Display_Bitsperpixel == 16)
            {
                SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
                SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
                SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
            }
            SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

            flags = SDL_OPENGL;
            if (Display_Fullscreen)
            {
                flags |= SDL_FULLSCREEN | SDL_NOFRAME;
            }
        }
        else
    #endif
        if (Display_Fullscreen)
        {
            flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN | SDL_NOFRAME;
        }
        else
        {
            flags = SDL_SWSURFACE;
        }

        Game_Screen = SDL_SetVideoMode (Display_Width, Display_Height, Display_Bitsperpixel, flags);
    }

    if (Game_Screen != NULL)
    {
        int mousex, mousey;

        SDL_ShowCursor(SDL_DISABLE);
        SDL_WM_SetCaption ("SDL Albion", NULL);

        if (Display_MouseLocked)
        {
            Game_OldCursor = SDL_GetCursor();
            SDL_SetCursor(Game_NoCursor);

            SDL_WM_GrabInput(SDL_GRAB_ON);

            SDL_WarpMouse(mouse_pos[1], mouse_pos[0]);
        }
        else
        {
            if (Display_Fullscreen)
            {
                Game_OldCursor = SDL_GetCursor();
                SDL_SetCursor(Game_NoCursor);

                SDL_WarpMouse(mouse_pos[1], mouse_pos[0]);
            }
            else
            {
                if (Game_MouseCursor == 2)
                {
                    Game_OldCursor = SDL_GetCursor();
                    SDL_SetCursor(Game_NoCursor);
                }
                else if (Game_MouseCursor == 1)
                {
                    Game_OldCursor = SDL_GetCursor();
                    SDL_SetCursor(Game_MinCursor);
                }

                SDL_GetMouseState(&mousex, &mousey);
                SDL_WarpMouse(mousex, mousey);
            }

            SDL_ShowCursor(SDL_ENABLE);
        }

    #ifdef ALLOW_OPENGL
        if (Game_UseOpenGL)
        {
            int index;

            glViewport(0, 0, Display_Width, Display_Height);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glGenTextures(3, &(Game_GLTexture[0]));

            for (index = 0; index < 3; index++)
            {
                glBindTexture(GL_TEXTURE_2D, Game_GLTexture[index]);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Render_Width, Render_Height, 0, GL_BGRA, (Display_Bitsperpixel == 32)?GL_UNSIGNED_INT_8_8_8_8_REV:GL_UNSIGNED_SHORT_5_6_5_REV, Game_TextureData);
            }

            Game_CurrentTexture = 0;
        }
    #endif
    }
#endif

    SDL_SemPost(Game_DisplaySem);
}

static void Game_Display_Destroy(int post)
{
    SDL_Rect rect;

    if (Game_OldCursor != NULL)
    {
        SDL_SetCursor(Game_OldCursor);
        Game_OldCursor = NULL;
    }

    /* clear screen */
#ifdef USE_SDL2
    SDL_SetRenderDrawColor(Game_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(Game_Renderer);
    SDL_RenderPresent(Game_Renderer);
#else
#ifdef ALLOW_OPENGL
    if (Game_UseOpenGL)
    {
        // do nothing
    }
    else
#endif
    {
        rect.x = 0;
        rect.y = 0;
        rect.w = Game_Screen->w;
        rect.h = Game_Screen->h;
        SDL_FillRect(Game_Screen, &rect, 0);
        SDL_Flip(Game_Screen);
    }
#endif

    SDL_ShowCursor(SDL_DISABLE);

#ifdef USE_SDL2
    SDL_SetRelativeMouseMode(SDL_FALSE);

    SDL_DestroyTexture(Game_Texture);
    Game_Texture = NULL;
    SDL_DestroyRenderer(Game_Renderer);
    Game_Renderer = NULL;
    SDL_DestroyWindow(Game_Window);
    Game_Window = NULL;
#else
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    //senquack - should not free screens allocated by setvideomode
//    SDL_FreeSurface(Game_Screen);
    Game_Screen = NULL;

#ifdef ALLOW_OPENGL
    if (Game_UseOpenGL)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(3, &(Game_GLTexture[0]));
    }
#endif
#endif

    if (post)
    {
        SDL_SemPost(Game_DisplaySem);
    }
}

void Game_CleanState(int imm)
{
    int i;
    SDL_Event event;

    for (i = 0; i < 256; i++)
    {
        if (Game_AllocatedMemory[i] != NULL)
        {
            free(Game_AllocatedMemory[i]);
            Game_AllocatedMemory[i] = NULL;
        }
    }

    for (i = 0; i < GAME_SAMPLE_CACHE_SIZE; i++)
    {
        if (Game_SampleCache[i] != NULL)
        {
            free(Game_SampleCache[i]);
            Game_SampleCache[i] = NULL;
        }
    }

    Game_ScreenWindow = Game_FrameBuffer;
    Game_ScreenWindowNum = 0;
    Game_DisplayStart = 0;
    Game_NextMemory = 0;

    Game_MQueueWrite = 0;
    Game_MQueueRead = 0;
    Game_KQueueWrite = 0;
    Game_KQueueRead = 0;
    Game_KBufferWrite = 0;
    Game_KBufferRead = 0;
    Game_LastKeyStroke = 0;

    memset(&Game_InterruptTable, 0, sizeof(Game_InterruptTable));
    memset(&Game_MouseTable, 0, sizeof(Game_MouseTable));
    memset(&Game_Palette_Or, 0, sizeof(Game_Palette_Or));

#ifdef USE_SDL2
    if (Game_Window != NULL)
#else
    if (Game_Screen != NULL)
#endif
    {
        if (imm)
        {
            Game_Display_Destroy(0);
        }
        else
        {
            event.type = SDL_USEREVENT;
            event.user.code = EC_DISPLAY_DESTROY;
            event.user.data1 = NULL;
            event.user.data2 = NULL;

            SDL_PushEvent(&event);

            SDL_SemWait(Game_DisplaySem);
        }
    }
}

static void Game_Cleanup(void)
{
    if (Game_Sound || Game_Music)
    {
        if (Game_Music)
        {
            if (Game_MidiSubsystem)
            {
                if (Game_MidiSubsystem <= 10)
                {
                    MidiPlugin_Shutdown();
                }
                else
                {
                    MidiPlugin2_Shutdown();
                }
            }
            else
            {
                Mix_HaltMusic();
            }
        }
        if (Game_Sound)
        {
            Mix_HaltChannel(-1);
        }
        Mix_CloseAudio();
    }

    Game_CleanState(1);

#if defined(ALLOW_OPENGL) || defined(USE_SDL2)
    if (Game_TextureData != NULL)
    {
        free(Game_TextureData);
        Game_TextureData = NULL;
    }
#endif

    if (Game_MidiDevice != NULL)
    {
        free(Game_MidiDevice);
        Game_MidiDevice = NULL;
    }

    if (Game_SoundFontPath != NULL)
    {
        free(Game_SoundFontPath);
        Game_SoundFontPath = NULL;
    }

    if (Temp_Font_Data != NULL)
    {
        free(Temp_Font_Data);
        Temp_Font_Data = NULL;
    }

    if (Albion_Font != NULL)
    {
        free(Albion_Font);
        Albion_Font = NULL;
    }

    if (Game_FlipSem != NULL)
    {
        SDL_DestroySemaphore(Game_FlipSem);
        Game_FlipSem = NULL;
    }

    if (Game_DisplaySem != NULL)
    {
        SDL_DestroySemaphore(Game_DisplaySem);
        Game_DisplaySem = NULL;
    }

    /*if (Game_ScreenWindow != NULL)
    {
        free(Game_ScreenWindow);
        Game_ScreenWindow = NULL;
    }*/

    if (Game_ScreenViewpartOriginal != NULL)
    {
        free(Game_ScreenViewpartOriginal);
        Game_ScreenViewpartOriginal = NULL;
    }

    if (Game_ScreenViewpartOverlay != NULL)
    {
        free(Game_ScreenViewpartOverlay);
        Game_ScreenViewpartOverlay = NULL;
    }

    if (Game_FrameBuffer != NULL)
    {
        free(Game_FrameBuffer);
        Game_FrameBuffer = NULL;
    }

    if (Game_MinCursor != NULL)
    {
        SDL_FreeCursor(Game_MinCursor);
        Game_MinCursor = NULL;
    }

    if (Game_NoCursor != NULL)
    {
        SDL_FreeCursor(Game_NoCursor);
        Game_NoCursor = NULL;
    }

    SDL_Quit();
}

static void Game_BuildRTable(void)
{
    int i;
    unsigned int err;

    for (i = 0; i < 256; i++) errno_rtable[i] = i;

    for (i = ERRNO_NUM - 1; i >= 0; i--)
    {
        err = errno_table[i];

        if (err < 256)
        {
            errno_rtable[err] = i;
        }
    }
}

static void Game_ReadCDPath(void)
{
    char str[8192];
    int len;
    FILE *f;

    f = Game_fopen("SETUP.INI", "rt");

    if (f != NULL)
    {
        while (!feof(f))
        {
            str[0] = 0;
            fscanf(f, "%8192[^\n]\n", str);
            if (strncasecmp(str, "SOURCE_PATH=", 12) == 0)
            {
                strcpy(Albion_CDPath, &(str[12]));
                len = strlen(Albion_CDPath);
                if ((len != 0) && (Albion_CDPath[len - 1] == '\r'))
                {
                    Albion_CDPath[len - 1] = 0;
                    len--;
                }
                if (len != 0)
                {
                    if (Albion_CDPath[len - 1] != '\\')
                    {
                        Albion_CDPath[len] = '\\';
                        Albion_CDPath[len + 1] = 0;
                    }
                    break;
                }
            }
        }
        fclose(f);
    }
}

static uint32_t calculate_crc(uint8_t *buf, unsigned int size)
{
#define POLYNOMIAL ((uint32_t)0xEDB88320)

    uint32_t crc_table[256];

    uint32_t i, j, crc_accum;

    for ( i = 0;  i < 256;  i++ )
    {
        crc_accum = i;
        for ( j = 0;  j < 8;  j++ )
        {
            if ( crc_accum & 1 )
                crc_accum = ( crc_accum >> 1 ) ^ POLYNOMIAL;
            else
                crc_accum = ( crc_accum >> 1 );
        }
        crc_table[i] = crc_accum;
    }

    crc_accum = (uint32_t)0xffffffff;

    for ( ; size != 0;  size-- )
    {
        crc_accum = ( crc_accum >> 8 ) ^ crc_table[(crc_accum ^ *buf++ ) & 0xff];
    }

    crc_accum ^= (uint32_t)0xffffffff;

    return crc_accum;
#undef POLYNOMIAL
}

static void Game_ReadFontData(void)
{
    char fname_base[256];
    uint8_t buf8[8];
    FILE *f;
    uint32_t size1, size2;

    strcpy(fname_base, Albion_CDPath);
    strcat(fname_base, "XLDLIBS\\FONTS0.XLD");

    f = Game_fopen(fname_base, "rb");
    if (f == NULL) return;

    fread(buf8, 1, 8, f);
    if (buf8[6] != 2)
    {
        fclose(f);
        return;
    }
    fread(&size1, 1, 4, f);
    fread(&size2, 1, 4, f);

    Albion_Font = (uint8_t *) malloc(size2);

    if (Albion_Font == NULL)
    {
        fclose(f);
        return;
    }

    fseek(f, size1, SEEK_CUR);
    fread(Albion_Font, 1, size2, f);

    fclose(f);

    switch (calculate_crc(Albion_Font, size2))
    {
        case 0x1183d03f:
            Albion_Font_Lang = AL_ENG_FRE;
            break;
        case 0x904d123e:
            Albion_Font_Lang = AL_CZE;
            break;
        case 0x992d4bd5:
            Albion_Font_Lang = AL_GER;
            break;
    }

    if (Font_Size_Shift > 0)
    {
        Temp_Font_Data = (uint8_t *) malloc((8*8) << (2*Font_Size_Shift + 1));
    }
    else
    {
        Temp_Font_Data = NULL;
    }
}

static int Game_Initialize(void)
{
    if (Game_Directory[0] == 0)
    {
        vfs_init(1);
    }
    else
    {
        char cur_dir[MAX_PATH];

        getcwd(cur_dir, MAX_PATH);
        chdir(Game_Directory);
        vfs_init(0);
        chdir(cur_dir);
    }

#ifdef USE_SDL2
    Game_Window = NULL;
    Game_Renderer = NULL;
    Game_Texture = NULL;
#else
    Game_Screen = NULL;
#endif
    Game_FrameBuffer = NULL;
    Game_ScreenWindow = NULL;
    memset(&Game_AllocatedMemory, 0, sizeof(Game_AllocatedMemory));

    Display_ChangeMode = 0;
    Font_Size_Shift = 0;


    Game_TimerRunning = 0;
    Game_TimerTick = 0;
    Game_TimerRun = 0;
    Game_VSyncTick = 0;
    Thread_Exited = 0;
    Thread_Exit = 0;
    Game_Paused = 0;
    Game_OldCursor = NULL;
    Game_NoCursor = NULL;
    Game_MinCursor = NULL;
    Game_NoCursorData = 0;
    Game_MinCursorData[0] = 0x50; // data
    Game_MinCursorData[1] = 0xA8;
    Game_MinCursorData[2] = 0x50;
    Game_MinCursorData[3] = 0xA8;
    Game_MinCursorData[4] = 0x50;
    Game_MinCursorData[5] = 0xD8; // mask
    Game_MinCursorData[6] = 0xF8;
    Game_MinCursorData[7] = 0x50;
    Game_MinCursorData[8] = 0xF8;
    Game_MinCursorData[9] = 0xD8;
    Game_MouseCursor = 0;
    Game_Sound = 1;
    Game_Music = 1;

    Game_VolumeDelta = 0;

    memset(&Game_SampleCache, 0, sizeof(Game_SampleCache));

    Game_SoundFontPath = NULL;
    Game_MidiDevice = NULL;


    //senquack - multiple config files now supported
    if (Game_ConfigFilename[0] == 0)
    {
        strcpy(Game_ConfigFilename, "Albion.cfg");				//default config file
    }


    Albion_CDPath[0] = 0;
    Albion_Font = NULL;
    Albion_Font_Lang = AL_UNKNOWN;
    Temp_Font_Data = NULL;
    Game_DisplaySem = NULL;
    Game_FlipSem = NULL;
    Game_ExitCode = 0;

    memset(&Game_OverlayDisplay, 0, sizeof(Game_OverlayInfo));
    memset(&Game_OverlayDraw, 0, sizeof(Game_OverlayInfo));

    Game_UseEnhanced3DEngine = 0;
    Game_UseEnhanced3DEngineNewValue = 0;
    Game_OverlayDisplay.Enhanced3DEngineUsed = -1;
    Game_OverlayDraw.Enhanced3DEngineUsed = -1;
    Game_ScreenViewpartOverlay = NULL;
    Game_ScreenViewpartOriginal = NULL;

    Game_ScreenshotFormat = 0;

#if defined(ALLOW_OPENGL) && !defined(USE_SDL2)
    Game_UseOpenGL = 0;
    Game_CurrentTexture = 0;
#endif
#if defined(ALLOW_OPENGL) || defined(USE_SDL2)
    Game_TextureData = NULL;
#endif

    Init_Display();
    Init_Audio();
    Init_Input();

    if ( SDL_Init (SDL_INIT_VIDEO //| SDL_INIT_TIMER
                    | ((Game_Joystick)?SDL_INIT_JOYSTICK:0)
                  ) != 0 )
    {
        fprintf (stderr, "Error: Couldn't initialize SDL: %s\n", SDL_GetError ());
        return -1;
    }

    Game_NoCursor = SDL_CreateCursor(&Game_NoCursorData, &Game_NoCursorData, 8, 1, 0, 0);

    if (Game_NoCursor == NULL)
    {
        fprintf (stderr, "Error: Couldn't create cursor: %s\n", SDL_GetError ());
        return -2;
    }

    Game_MinCursor = SDL_CreateCursor(&(Game_MinCursorData[0]), &(Game_MinCursorData[5]), 8, 5, 2, 2);

    if (Game_MinCursor == NULL)
    {
        fprintf (stderr, "Error: Couldn't create cursor: %s\n", SDL_GetError ());
        Game_Cleanup();
        return -2;
    }

    if (Game_Joystick)
    {
        SDL_JoystickOpen(0);
    }

    Game_FrameBuffer = (uint8_t *) malloc(360*481);
    if (Game_FrameBuffer == NULL)
    {
        fprintf(stderr, "Error: Not enough memory\n");
        Game_Cleanup();
        return -3;
    }

    if (Game_UseEnhanced3DEngineNewValue)
    {
        Game_ScreenViewpartOverlay = (uint8_t *) malloc(800*384*2);
        if (Game_ScreenViewpartOverlay == NULL)
        {
            fprintf(stderr, "Error: Not enough memory\n");
            Game_Cleanup();
            return -4;
        }

        Game_ScreenViewpartOriginal = (uint8_t *) malloc(360*192*2);
        if (Game_ScreenViewpartOriginal == NULL)
        {
            fprintf(stderr, "Error: Not enough memory\n");
            Game_Cleanup();
            return -4;
        }
    }

    Game_OverlayDisplay.ScreenViewpartOverlay = Game_OverlayDraw.ScreenViewpartOverlay = Game_ScreenViewpartOverlay;
    Game_OverlayDisplay.ScreenViewpartOriginal = Game_OverlayDraw.ScreenViewpartOriginal = Game_ScreenViewpartOriginal;

/*	Game_ScreenWindow = (uint8_t *) malloc(65536);
    if (Game_ScreenWindow == NULL)
    {
        fprintf(stderr, "Error: Not enough memory\n");
        return -4;
    }*/

    Game_DisplaySem = SDL_CreateSemaphore(0);
    if (Game_DisplaySem == NULL)
    {
        fprintf(stderr, "Error: Unable to create semaphore\n");
        Game_Cleanup();
        return -5;
    }

    Game_FlipSem = SDL_CreateSemaphore(0);
    if (Game_FlipSem == NULL)
    {
        fprintf(stderr, "Error: Unable to create semaphore\n");
        Game_Cleanup();
        return -6;
    }

    Game_BuildRTable();

    Game_stdin = stdin;
    Game_stdout = stdout;
    Game_stderr = stderr;

    return 0;
}

static void Game_Initialize2(void)
{
    uint32_t Sound, Music;
    int frequency, channels;
    Uint16 format;


    if (Game_Sound || Game_Music)
    {
        Sound = Game_Sound;
        Music = Game_Music;
        Game_Sound = 0;
        Game_Music = 0;

        if ( SDL_InitSubSystem(SDL_INIT_AUDIO) == 0 )
        {
            frequency = Game_AudioRate;
            format = Game_AudioFormat;
            channels = Game_AudioChannels;

            if ( Mix_OpenAudio(frequency, format, channels, Game_AudioBufferSize) == 0)
            {
                if ( Mix_QuerySpec(&frequency, &format, &channels) )
                {
#if defined(__DEBUG__)
                    fprintf(stderr, "Audio rate: %i\n", frequency);
                    fprintf(stderr, "Audio format: 0x%x\n", format);
                    fprintf(stderr, "Audio channels: %i\n", channels);
#endif
                    Game_AudioRate = frequency;
                    Game_AudioFormat = format;
                    Game_AudioChannels = channels;
                }

                Game_MixerChannels = 0;
                Mix_AllocateChannels(GAME_MIXER_CHANNELS_INITIAL);

                Game_Sound = Sound;
                Game_Music = Music;

                if (Game_Music && Game_MidiSubsystem)
                {
                    if (Game_MidiSubsystem <= 10)
                    {
                        if (MidiPlugin_Startup())
                        {
                            Game_Music = 0;
                        }
                    }
                    else
                    {
                        if (MidiPlugin2_Startup())
                        {
                            Game_Music = 0;
                        }
                    }
                }

                Game_SoundMasterVolume = (Game_Sound)?127:0;
                Game_MusicMasterVolume = (Game_Music)?127:0;
            }
            else
            {
                SDL_QuitSubSystem(SDL_INIT_AUDIO);
            }
        }
    }

//senquack - Moved this to Initialize2 because of changes to tvout (we must read
//				the configuration settings first, now)


    Init_Display2();
    Init_Audio2();
    Init_Input2();

#if !defined(USE_SDL2)
    SDL_EnableUNICODE(1);
#endif

    Game_VideoAspectX = (360 << 16) / Picture_Width;
    Game_VideoAspectY = (240 << 16) / Picture_Height;

    Game_VideoAspectXR = (Picture_Width << 16) / 360;
    Game_VideoAspectYR = (Picture_Height << 16) / 240;

#if defined(ALLOW_OPENGL) || defined(USE_SDL2)
#if !defined(USE_SDL2)
    if (Game_UseOpenGL)
#endif
    {
        Game_TextureData = malloc(Render_Width * Render_Height * Display_Bitsperpixel / 8);
    }
#endif
}

static void Game_Event_Loop(void)
{
    SDL_Thread *MainThread;
    SDL_Thread *FlipThread;
    SDL_Thread *TimerThread;
    SDL_Event event;
    uint32_t AppMouseFocus;
    uint32_t AppInputFocus;
    uint32_t AppActive;
    int FlipActive, CreateAfterFlip, DestroyAfterFlip;


    TimerThread = SDL_CreateThread(
        Game_TimerThread,
#ifdef USE_SDL2
        "timer",
#endif
        NULL
    );

    if (TimerThread == NULL)
    {
        fprintf(stderr, "Error: Unable to start timer thread\n");
        return;
    }

    FlipThread = SDL_CreateThread(
        Game_FlipThread,
#ifdef USE_SDL2
        "flip",
#endif
        NULL
    );

    if (FlipThread == NULL)
    {
        fprintf(stderr, "Error: Unable to start flip thread\n");

        Thread_Exited = 1;
        Thread_Exit = 1;

        SDL_WaitThread(TimerThread, NULL);

        return;
    }

    MainThread = SDL_CreateThread(
        Game_MainThread,
#ifdef USE_SDL2
        "main",
#endif
        NULL
    );

    if (MainThread == NULL)
    {
        fprintf(stderr, "Error: Unable to start main thread\n");

        Thread_Exited = 1;
        Thread_Exit = 1;

        SDL_SemPost(Game_FlipSem);

        SDL_WaitThread(FlipThread, NULL);
        SDL_WaitThread(TimerThread, NULL);

        return;
    }

#ifdef USE_SDL2
    AppMouseFocus = 1;
    AppInputFocus = 1;
    AppActive = 1;
#else
    {
        uint32_t AppState;

        AppState = SDL_GetAppState();

        AppMouseFocus = AppState & SDL_APPMOUSEFOCUS;
        AppInputFocus = AppState & SDL_APPINPUTFOCUS;
        AppActive = AppState & SDL_APPACTIVE;
    }
#endif

    FlipActive = 0;
    CreateAfterFlip = 0;
    DestroyAfterFlip = 0;

    while (!Thread_Exited && SDL_WaitEvent(&event))
    {
        if (!Handle_Input_Event(&event))
        switch(event.type)
        {
        #ifdef USE_SDL2
            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_ENTER:
                        AppMouseFocus = 1;
                        break;
                    case SDL_WINDOWEVENT_LEAVE:
                        AppMouseFocus = 0;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        AppInputFocus = 1;
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        AppInputFocus = 0;
                        break;
                    case SDL_WINDOWEVENT_MINIMIZED:
                        AppActive = 0;
                        break;
                    case SDL_WINDOWEVENT_MAXIMIZED:
                    case SDL_WINDOWEVENT_RESTORED:
                        AppActive = 1;
                        break;
                    //case SDL_WINDOWEVENT_CLOSE:
                    // todo: ?
                }

                break;
                // case SDL_WINDOWEVENT:
        #else
            case SDL_ACTIVEEVENT:
                if (event.active.state & SDL_APPMOUSEFOCUS)
                {
                    AppMouseFocus = event.active.gain;
                }
                if (event.active.state & SDL_APPINPUTFOCUS)
                {
                    AppInputFocus = event.active.gain;
                }
                if (event.active.state & SDL_APPACTIVE)
                {
                    AppActive = event.active.gain;
                }

                break;
                // case SDL_ACTIVEEVENT:
        #endif

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (
                #ifdef USE_SDL2
                    Game_Window != NULL
                #else
                    Game_Screen != NULL
                #endif
                    && AppActive && AppInputFocus)
                {
                    if ( ( (Game_KQueueWrite + 1) & (GAME_KQUEUE_LENGTH - 1) ) == Game_KQueueRead )
                    {
#if defined(__DEBUG__)
                        printf("keyboard event queue overflow\n");
#endif
                    }
                    else
                    {
                        Game_EventKQueue[Game_KQueueWrite] = event;

                        Game_KQueueWrite = (Game_KQueueWrite + 1) & (GAME_KQUEUE_LENGTH - 1);
                    }
                }

                break;
                // case SDL_KEYDOWN, SDL_KEYUP:
            case SDL_MOUSEMOTION:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
        #ifdef USE_SDL2
            case SDL_MOUSEWHEEL:
        #endif
                if (
                #ifdef USE_SDL2
                    Game_Window != NULL
                #else
                    Game_Screen != NULL
                #endif
                    && AppActive && AppInputFocus && AppMouseFocus)
                {

                    if ( ( (Game_MQueueWrite + 1) & (GAME_MQUEUE_LENGTH - 1) ) == Game_MQueueRead )
                    {
#if defined(__DEBUG__)
                        printf("mouse event queue overflow\n");
#endif
                    }
                    else
                    {
                        Game_EventMQueue[Game_MQueueWrite] = event;

                        Game_MQueueWrite = (Game_MQueueWrite + 1) & (GAME_MQUEUE_LENGTH - 1);
                    }
                }

                break;
                // case SDL_MOUSEMOTION, SDL_MOUSEBUTTONUP, SDL_MOUSEBUTTONDOWN:
            case SDL_QUIT:
                /* todo: question */

                if (Thread_Exit) exit(1);

                Thread_Exit = 1;

                SDL_SemPost(Game_FlipSem);

                SDL_WaitThread(FlipThread, NULL);

                SDL_WaitThread(MainThread, NULL);

                SDL_WaitThread(TimerThread, NULL);

                break;
                // case SDL_QUIT:
            case SDL_USEREVENT:
                switch (event.user.code)
                {
                    case EC_DISPLAY_CREATE:
                        if (FlipActive)
                        {
                            CreateAfterFlip = 1;
                        }
                        else
                        {
                            Game_Display_Create();
                        }

                        break;
                        // case EC_DISPLAY_CREATE:

                    case EC_DISPLAY_DESTROY:
                        if (FlipActive)
                        {
                            DestroyAfterFlip = 1;
                        }
                        else
                        {
                            Game_Display_Destroy(1);
                        }

                        break;
                        // case EC_DISPLAY_DESTROY:

                    case EC_DISPLAY_FLIP_START:
                        if (!FlipActive &&
                        #ifdef USE_SDL2
                            Game_Window != NULL
                        #else
                            Game_Screen != NULL
                        #endif
                            )
                        {
                            FlipActive = 1;

                            /* ??? */
/*								SDL_LockSurface(Game_Screen);*/

                            SDL_SemPost(Game_FlipSem);
                        }

                        break;
                        // case EC_DISPLAY_FLIP_START:

                    case EC_DISPLAY_FLIP_FINISH:
                        if (FlipActive)
                        {
                        #if defined(USE_SDL2)
                            SDL_UpdateTexture(Game_Texture, NULL, Game_TextureData, Render_Width * Display_Bitsperpixel / 8);
                            SDL_RenderCopy(Game_Renderer, Game_Texture, NULL, NULL);
                            SDL_RenderPresent(Game_Renderer);
                        #elif defined(ALLOW_OPENGL)
                            if (Game_UseOpenGL)
                            {
                                glBindTexture(GL_TEXTURE_2D, Game_GLTexture[Game_CurrentTexture]);

                                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Render_Width, Render_Height, GL_BGRA, (Display_Bitsperpixel == 32)?GL_UNSIGNED_INT_8_8_8_8_REV:GL_UNSIGNED_SHORT_5_6_5_REV, Game_TextureData);

                                glEnable(GL_TEXTURE_2D);

                                static const GLfloat QuadVertices[2*4] = {
                                    -1.0f,  1.0f,
                                     1.0f,  1.0f,
                                     1.0f, -1.0f,
                                    -1.0f, -1.0f,
                                };
                                static const GLfloat QuadTexCoords[2*4] = {
                                    0.0f, 0.0f,
                                    1.0f, 0.0f,
                                    1.0f, 1.0f,
                                    0.0f, 1.0f
                                };

                                glEnableClientState(GL_VERTEX_ARRAY);
                                glEnableClientState(GL_TEXTURE_COORD_ARRAY);

                                glVertexPointer(2, GL_FLOAT, 0, &(QuadVertices[0]));
                                glTexCoordPointer(2, GL_FLOAT, 0, &(QuadTexCoords[0]));
                                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

                                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                                glDisableClientState(GL_VERTEX_ARRAY);

                                glDisable(GL_TEXTURE_2D);

                                SDL_GL_SwapBuffers();

                                Game_CurrentTexture++;
                                if (Game_CurrentTexture > 2)
                                {
                                    Game_CurrentTexture = 0;
                                }
                            }
                        #endif

                            /* ??? */
/*								SDL_UnlockSurface(Game_Screen);
                            SDL_Flip(Game_Screen);*/

                            FlipActive = 0;

                            if (DestroyAfterFlip)
                            {
                                Game_Display_Destroy(1);
                                DestroyAfterFlip = 0;
                            }

                            if (CreateAfterFlip)
                            {
                                Game_Display_Create();
                                CreateAfterFlip = 0;
                            }
                        }

                        break;
                        // case EC_DISPLAY_FLIP_FINISH:
                    case EC_PROGRAM_QUIT:
                        if (Thread_Exit) exit(1);

                        Thread_Exit = 1;

                        SDL_SemPost(Game_FlipSem);

                        SDL_WaitThread(FlipThread, NULL);

                        SDL_WaitThread(MainThread, NULL);

                        SDL_WaitThread(TimerThread, NULL);

                        break;
                        // case EC_PROGRAM_QUIT:
                    case EC_MOUSE_MOVE:
                        {
                            int mousex, mousey;

                            SDL_GetMouseState(&mousex, &mousey);
                        #ifdef USE_SDL2
                            SDL_WarpMouseInWindow(Game_Window, mousex + (int) event.user.data1, mousey + (int) event.user.data2);
                        #else
                            SDL_WarpMouse(mousex + (int) event.user.data1, mousey + (int) event.user.data2);
                        #endif
                        }
                        break;
                    case EC_MOUSE_SET:
                        {
                        #ifdef USE_SDL2
                            SDL_WarpMouseInWindow(Game_Window, (int) event.user.data1, (int) event.user.data2);
                        #else
                            SDL_WarpMouse((int) event.user.data1, (int) event.user.data2);
                        #endif
                        }
                        break;
                }

                break;
                // case SDL_USEREVENT:
            default:
                Handle_Input_Event2(&event);
                break;
        } // switch(event.type)
    }

}

int main (int argc, char *argv[])
{
    Game_ConfigFilename[0] = 0;
    Game_Directory[0] = 0;

    //senquack - can now specify config file on command line
    // read parameters
    if (argc > 1)
    {
        int numpar;
        char **param;

        numpar = argc;
        param = argv;

        while (numpar)
        {
            //senquack - new config option for specifying a config file to use
            if ( strncmp(*param, "--config-file", 13) == 0)
            {
                (*param) += 14;	// skip space or equals
                if ((strlen(*param)) > 0 && (strlen(*param) < 80))
                {
                    strcpy(Game_ConfigFilename, *param);
                }
            }
            else if ( strncmp(*param, "--game-dir", 10) == 0)
            {
                (*param) += 11;	// skip space or equals
                if ((strlen(*param)) > 0 && (strlen(*param) < 80))
                {
                    strcpy(Game_Directory, *param);
                }
            }

            numpar--;
            param++;
        }
    }

#if defined(__DEBUG__)
    fprintf(stderr, "Initializing...\n");
#endif
    // initialization
    {
        int return_value;

        return_value = Game_Initialize();
        if (return_value) return return_value;
    }

    Game_ReadConfig();
    Game_ReadCDPath();
    Game_ReadFontData();

    Game_Initialize2();

#if defined(__DEBUG__)
    fprintf(stderr, "Starting game event loop...\n");
#endif
    Game_Event_Loop();

#if defined(__DEBUG__)
    fprintf(stderr, "Cleaning up...\n");
#endif
    Game_Cleanup();

    Cleanup_Input();
    Cleanup_Audio();
    Cleanup_Display();

    return Game_ExitCode;
}

