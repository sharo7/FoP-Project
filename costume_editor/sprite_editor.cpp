#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

const int scaler = 8;
const int Left_toolbar_width = 240;
const int window_width = 1600,window_height=900;
const int canvas_height = 120;
const int canvas_width = 170;

struct Color
{
    Uint8 red, green, blue, alpha;
};

const Color WHITE = {255,255,255,255};
const Color BLACK = {0,0,0,255};
const Color RED = {255,0,0,255};
const Color BLUE = {0,0,255,255};
const Color CYAN = {0,255,255,255};
const Color MAGENTA = {255,0,255,255};
const Color YELLOW = {255,255,0,255};
const Color GRAY = {128,128,128,128};

const Color Default_Canvas_Color = WHITE;

struct Canvas
{
    Color Canvas_Pixels[canvas_height][canvas_width];
};

struct Point
{
    int x,y;
};

const int NONE_UI_ID=-1;
const int SLIDER_RED_UI_ID=0;
const int SLIDER_GREEN_UI_ID=1;
const int SLIDER_BLUE_UI_ID=2;
const int File_Input_UI_ID=3;
const int File_Load_UI_ID=4;

struct UI_State
{
    bool mouse_pressed_on_ui;
    int active_element;
    string filename;
    bool Is_text_input_Active;
};

struct App_State
{
    Canvas canvas;
    int current_Active_tool;
    Color active_brush_color;
    Color background_color;
    bool mouse_pressed_on_canvas;
    Point starting_point;
    UI_State UI;
    TTF_Font *font;
};

const int Brush_tool_id=0;
const int Fill_Bucket_tool_id=1;
const int Eraser_tool_id=2;
const int line_tool_id=3;
const int rect_tool_id=4;
const int circle_tool_id=5;


////////////////functions///////////////////////////////

void set_pixel_color(Canvas& canvas, int x, int y, Color color)
{
    if (x >= 0 && x < canvas_width && y >= 0 && y < canvas_height)
    {
        canvas.Canvas_Pixels[y][x] = color;
    }
}

Color Get_pixel_color(Canvas& canvas, int x, int y)
{
    if (x >= 0 && x < canvas_width && y >= 0 && y < canvas_height)
    {
        return canvas.Canvas_Pixels[y][x];
    }
    else
    {
        return WHITE;
    };
}

void Fill_Bucket(Canvas& canvas, int x_start, int y_start, Color new_color)
{
    if (x_start >= 0 && x_start < canvas_width && y_start >= 0 && y_start < canvas_height)
    {
        Color target_color = Get_pixel_color(canvas, x_start, y_start);
        if (target_color.red == new_color.red && target_color.green == new_color.green && target_color.blue == new_color.blue && target_color.alpha == new_color.alpha)
        {
            return;
        }
        else
        {
            vector<Point> pixel_check_list;
            pixel_check_list.push_back({x_start, y_start});

            while (pixel_check_list.empty()==false)
            {
                Point point = pixel_check_list.back();
                pixel_check_list.pop_back();

                if (point.x < canvas_width && point.x >= 0  && point.y < canvas_height && point.y >= 0)
                {
                    Color current_color = Get_pixel_color(canvas, point.x, point.y);
                    if (current_color.red == target_color.red && current_color.green == target_color.green && current_color.blue == target_color.blue && current_color.alpha == target_color.alpha)
                    {
                        set_pixel_color(canvas, point.x, point.y, new_color);
                        pixel_check_list.push_back({point.x +1, point.y});
                        pixel_check_list.push_back({point.x -1, point.y});
                        pixel_check_list.push_back({point.x, point.y +1});
                        pixel_check_list.push_back({point.x, point.y -1});
                    }
                }
            }

        }

    }
}

void Draw_Line(Canvas& canvas, int x_start, int y_start, int x_end, int y_end,Color new_color)
{
    int Dx = abs(x_end - x_start);
    int Dy = -abs(y_end - y_start);
    int Step_X, Step_Y;
    if (x_end > x_start)
    {
        Step_X = 1;
    }
    else
    {
        Step_X = -1;
    }

    if (y_end > y_start)
    {
        Step_Y = 1;
    }
    else
    {
        Step_Y = -1;
    }

    int error_amount = Dx + Dy ;

    while (1) {
        set_pixel_color(canvas, x_start, y_start, new_color);
        if (x_start == x_end && y_start == y_end)
        {
            break;
        }
        if (error_amount * 2 >= Dy)
        {
            error_amount += Dy;
            x_start += Step_X;
        }
        if (error_amount * 2 <= Dx)
        {
            error_amount += Dx;
            y_start += Step_Y;
        }
    }
}

void Draw_Rectangle(Canvas& canvas, int x_start, int y_start, int x_end, int y_end,Color new_color)
{
    if (x_end < x_start) swap(x_end, x_start);
    if (y_end < y_start) swap(y_end, y_start);
    for (int i=x_start; i<=x_end; i++)
    {
        set_pixel_color(canvas,i,y_start, new_color);
        set_pixel_color(canvas,i,y_end, new_color);
    }

    for (int i=y_start; i<=y_end; i++)
    {
        set_pixel_color(canvas,x_start,i, new_color);
        set_pixel_color(canvas,x_end,i, new_color);
    }
}


void Draw_Circle(Canvas& canvas, int x_Center_start, int y_Center_start, int radius,Color new_color)
{
    int x=radius, y=0;
    int P = 1-radius;

    set_pixel_color(canvas, x_Center_start + x, y_Center_start, new_color);
    set_pixel_color(canvas, x_Center_start - x, y_Center_start, new_color);
    set_pixel_color(canvas, x_Center_start, y_Center_start + x, new_color);
    set_pixel_color(canvas, x_Center_start, y_Center_start - x, new_color);

    if (radius > 0)
    {
        while (x>y)
        {
            y++;
            if (P <= 0 )
            {
                P += 2*y +1;

            }
            else
            {
                x--;
                P += 2*y +1;
                P -= 2*x;
            }

            set_pixel_color(canvas, x_Center_start+x, y_Center_start+y, new_color);
            set_pixel_color(canvas, x_Center_start-x, y_Center_start+y, new_color);
            set_pixel_color(canvas, x_Center_start+x, y_Center_start-y, new_color);
            set_pixel_color(canvas, x_Center_start-x, y_Center_start-y, new_color);
            set_pixel_color(canvas, x_Center_start+y, y_Center_start+x, new_color);
            set_pixel_color(canvas, x_Center_start-y, y_Center_start+x, new_color);
            set_pixel_color(canvas, x_Center_start+y, y_Center_start-x, new_color);
            set_pixel_color(canvas, x_Center_start-y, y_Center_start-x, new_color);


        }
    }

}

void Canvas_to_Buffer_Convertor(const Canvas& canvas, Uint32* buffer)
{
    for (int y = 0;y < canvas_height ;y++)
        {
            for (int x = 0;x < canvas_width;x++)
            {
                Color current_pixel = canvas.Canvas_Pixels[y][x];
                buffer[y*canvas_width+x] = current_pixel.red << 24 | current_pixel.green << 16 | current_pixel.blue << 8 | current_pixel.alpha;
            }
        }
}

Point mouse_position_to_canvas_convertor(int mouse_x, int mouse_y)
{
    if (mouse_x >= Left_toolbar_width)
    {
        return {(mouse_x-Left_toolbar_width) / scaler, mouse_y / scaler};
    }
    else
    {
        return {-1,-1};
    }
}

bool Png_Loader_On_Canvas(Canvas& canvas,const char* file_name)
{
    SDL_Surface* surface = IMG_Load(file_name);
    if (!surface)
    {
        return (false);
    }

    SDL_Surface* scaled_surface = SDL_CreateRGBSurfaceWithFormat(0,canvas_width,canvas_height,32,SDL_PIXELFORMAT_RGBA8888);
    if (!scaled_surface)
    {
        SDL_FreeSurface(surface);
        return (false);
    }

    SDL_BlitScaled(surface,NULL,scaled_surface,NULL);

    SDL_LockSurface(scaled_surface);
    Uint32* pixels = (Uint32*)scaled_surface->pixels;
    for (int i=0;i<canvas_height;i++)
    {
        for (int j=0;j<canvas_width;j++)
        {
            Uint32 pixel = pixels[i*canvas_width+j];
            Uint8 red = (pixel >> 24) & 0b11111111;
            Uint8 green = (pixel >> 16) & 0b11111111;
            Uint8 blue = (pixel >> 8) & 0b11111111;
            Uint8 alpha = pixel & 0b11111111;
            canvas.Canvas_Pixels[i][j]={red,green,blue,alpha};
        }
    }
    SDL_UnlockSurface(scaled_surface);

    SDL_FreeSurface(scaled_surface);
    SDL_FreeSurface(surface);
    return (true);

}

bool Png_Saver_From_Canvas(Canvas& canvas,const char* file_name)
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0,canvas_width,canvas_height,32,SDL_PIXELFORMAT_RGBA8888);
    if (!surface)
    {
        return (false);
    }

    SDL_LockSurface(surface);
    Uint32* pixels = (Uint32*)surface->pixels;
    for (int i=0;i<canvas_height;i++)
    {
        for (int j=0;j<canvas_width;j++)
        {
            Color current_pixel = canvas.Canvas_Pixels[i][j];
            pixels[i*canvas_width+j] = current_pixel.red<<24 | current_pixel.green<<16 | current_pixel.blue<<8 | current_pixel.alpha;
        }
    }
    SDL_UnlockSurface(surface);

    int saved_img = IMG_SavePNG(surface,file_name);
    SDL_FreeSurface(surface);
    return (saved_img ==0 ? true : false);
}

void text_render(SDL_Renderer* renderer, TTF_Font* font,const char* text,int x,int y,SDL_Color new_color,bool Is_Centered=false)
{
    if (!font or !text or strlen(text)==0)
        return;
    else{
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font,text,new_color);
        if (!surface) return;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer,surface);
        if (!texture)return;
        else
        {
            SDL_Rect destination={x,y,surface->w,surface->h};
            if (Is_Centered)
            {
                destination.x -= surface->w/2;
                destination.y -= surface->h/2;
            }
            SDL_RenderCopy(renderer,texture,NULL,&destination);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout << "SDL_Failed: " << SDL_GetError() << endl;
        return 1;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        cout<< "IMG_SDL failed: " << IMG_GetError() << endl;
        SDL_Quit();
        return 1;
    }
    if (TTF_Init() < 0)
    {
        cout<< "TTF_SDL failed: " << TTF_GetError() << endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }


    SDL_Window* window = SDL_CreateWindow("sprite_editor",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,window_width,window_height,SDL_WINDOW_SHOWN);

    if (!window)
    {
        cout<< "SDL_CreateWindow failed: " << SDL_GetError() << endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        cout<< "SDL_CreateRenderer failed: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STREAMING,canvas_width,canvas_height);
    if (!texture)
    {
        cout<< "SDL_CreateTexture failed: " << SDL_GetError() << endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = TTF_OpenFont("./Fonts/arial.ttf",16);
    if (!font)
    {
        cout<< "TTF_OpenFont failed: " << TTF_GetError() << endl;
        cout<<"no text is can be visable"<<endl;
    }

    App_State state;
    for (int i=0;i<canvas_height;i++)
    {
        for (int j=0;j<canvas_width;j++)
        {
            state.canvas.Canvas_Pixels[i][j]= Default_Canvas_Color;
        }
    }

    state.current_Active_tool = Brush_tool_id;
    state.active_brush_color = BLACK;
    state.background_color = Default_Canvas_Color;
    state.mouse_pressed_on_canvas = false;
    state.starting_point = {0,0};

    state.UI.mouse_pressed_on_ui = false;
    state.UI.active_element = NONE_UI_ID;
    state.UI.filename.clear();
    state.UI.Is_text_input_Active = false;

    state.font=font;

    SDL_StartTextInput();




    SDL_Rect Red_Slider_Rect = {20,30,200,20};
    SDL_Rect Green_Slider_Rect = {20,70,200,20};
    SDL_Rect Blue_Slider_Rect = {20,110,200,20};
    SDL_Rect Color_Previewer_Rect = {20,150,40,40};

    SDL_Rect Brush_Button = {20,200,60,30};
    SDL_Rect Fill_Bucket_Button = {90,200,60,30};
    SDL_Rect Eraser_Button = {160,200,60,30};
    SDL_Rect Line_Draw_Button = {20,240,60,30};
    SDL_Rect Rectangle_Draw_Button = {90,240,60,30};
    SDL_Rect Circle_Draw_Button = {160,240,60,30};

    SDL_Rect Reset_Canvas_Button = {20,280,200,30};

    SDL_Rect File_Name_Inputer = {20,330,200,30};
    SDL_Rect Load_File_Button = {20,370,200,30};
    SDL_Rect Save_File_Button = {20,410,200,30};


    bool Is_Running = true;
    SDL_Event event;

    while (Is_Running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type==SDL_QUIT) {
                Is_Running = false;
            }
            else if (event.type==SDL_KEYDOWN)
            {

                switch (event.key.keysym.sym)
                {
                    case SDLK_1:
                    {
                        state.current_Active_tool = Brush_tool_id;
                        break;
                    }
                    case SDLK_2:
                    {
                        state.current_Active_tool = Fill_Bucket_tool_id;
                        break;
                    }
                    case SDLK_3:
                    {
                        state.current_Active_tool = Eraser_tool_id;
                        break;
                    }
                    case SDLK_4:
                    {
                        state.current_Active_tool = line_tool_id;
                        break;
                    }
                    case SDLK_5:
                    {
                        state.current_Active_tool = rect_tool_id;
                        break;
                    }
                    case SDLK_6:
                    {
                        state.current_Active_tool = circle_tool_id;
                        break;
                    }
                    case SDLK_c:
                    {
                        for (int i=0;i<canvas_height;i++)
                            for (int j=0;j<canvas_width;j++)
                                state.canvas.Canvas_Pixels[i][j] = Default_Canvas_Color;


                        break;
                    }
                    break;
                }

                if (state.UI.Is_text_input_Active)
                {
                    if (event.key.keysym.sym == SDLK_RETURN)
                    {
                        state.UI.Is_text_input_Active = false;
                    }
                    else if (event.key.keysym.sym == SDLK_BACKSPACE)
                    {
                        if (!state.UI.filename.empty())
                        {
                            state.UI.filename.pop_back();
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        state.UI.Is_text_input_Active = false;
                    }
                }

                SDL_SetWindowTitle(window, "Sprite_Editor");

            }
            else if (event.type==SDL_TEXTINPUT)
            {
                if (state.UI.Is_text_input_Active)
                {
                    state.UI.filename += event.text.text;
                }
            }
            else if (event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                int mouse_x = event.button.x;
                int mouse_y = event.button.y;
                SDL_Point mouse_point = {mouse_x, mouse_y};

                if (mouse_x < Left_toolbar_width)
                {
                    if (SDL_PointInRect(&mouse_point,&Red_Slider_Rect))
                    {
                        state.UI.active_element = SLIDER_RED_UI_ID;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Green_Slider_Rect))
                    {
                        state.UI.active_element = SLIDER_GREEN_UI_ID;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Blue_Slider_Rect))
                    {
                        state.UI.active_element = SLIDER_BLUE_UI_ID;
                        state.UI.mouse_pressed_on_ui = true;
                    }


                    else if (SDL_PointInRect(&mouse_point,&Brush_Button))
                    {
                        state.current_Active_tool = Brush_tool_id;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Fill_Bucket_Button))
                    {
                        state.current_Active_tool = Fill_Bucket_tool_id;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Eraser_Button))
                    {
                        state.current_Active_tool = Eraser_tool_id;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Line_Draw_Button))
                    {
                        state.current_Active_tool = line_tool_id;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Rectangle_Draw_Button))
                    {
                        state.current_Active_tool = rect_tool_id;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Circle_Draw_Button))
                    {
                        state.current_Active_tool = circle_tool_id;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Reset_Canvas_Button))
                    {
                        for (int i=0;i<canvas_height;i++)
                            for (int j=0;j<canvas_width;j++)
                                state.canvas.Canvas_Pixels[i][j] = Default_Canvas_Color;

                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&File_Name_Inputer))
                    {
                        state.UI.Is_text_input_Active = true;
                        state.UI.active_element= File_Input_UI_ID;
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Load_File_Button))
                    {
                        if (!state.UI.filename.empty())
                        {
                            if (!Png_Loader_On_Canvas(state.canvas,state.UI.filename.c_str())) {
                                cout<<"Cannot load file"<<state.UI.filename.c_str()<<endl;
                            }
                        }

                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else if (SDL_PointInRect(&mouse_point,&Save_File_Button))
                    {
                        if (!state.UI.filename.empty())
                        {
                            if (!Png_Saver_From_Canvas(state.canvas, state.UI.filename.c_str()))
                            {
                                cout<<"Cannot save file"<<state.UI.filename.c_str()<<endl;
                            }
                        }
                        state.UI.mouse_pressed_on_ui = true;
                    }
                    else
                    {
                        state.UI.mouse_pressed_on_ui = false;
                    }
                }
                else
                {
                    Point canvas_point = mouse_position_to_canvas_convertor(mouse_x,mouse_y);
                    if (canvas_point.x >= 0 && canvas_point.y >= 0)
                    {
                        state.mouse_pressed_on_canvas = true;
                        state.starting_point=canvas_point;

                        if (state.current_Active_tool == Brush_tool_id)
                            set_pixel_color(state.canvas,canvas_point.x,canvas_point.y,state.active_brush_color);
                        else if (state.current_Active_tool == Eraser_tool_id)
                            set_pixel_color(state.canvas,canvas_point.x,canvas_point.y,state.background_color);
                        else if (state.current_Active_tool == Fill_Bucket_tool_id)
                            Fill_Bucket(state.canvas,canvas_point.x,canvas_point.y,state.active_brush_color);

                    }
                }
            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            {
                if (state.mouse_pressed_on_canvas == true)
                {
                    Point mouse_end_point = mouse_position_to_canvas_convertor(event.button.x,event.button.y);
                    if (mouse_end_point.x >= 0 && mouse_end_point.y >= 0)
                    {
                        if (state.current_Active_tool == line_tool_id)
                        {
                            Draw_Line(state.canvas,state.starting_point.x, state.starting_point.y,mouse_end_point.x, mouse_end_point.y,state.active_brush_color);
                        }
                        else if (state.current_Active_tool == rect_tool_id)
                        {
                            Draw_Rectangle(state.canvas,state.starting_point.x,state.starting_point.y,mouse_end_point.x,mouse_end_point.y,state.active_brush_color);
                        }
                        else if (state.current_Active_tool == circle_tool_id)
                        {
                            int radius = int(sqrt(pow((mouse_end_point.x-state.starting_point.x),2)+pow((mouse_end_point.y-state.starting_point.y),2)));
                            Draw_Circle(state.canvas,state.starting_point.x,state.starting_point.y,radius,state.active_brush_color);
                        }
                    }
                    state.mouse_pressed_on_canvas = false;
                }
                if (state.UI.active_element >= SLIDER_RED_UI_ID && state.UI.active_element <= SLIDER_BLUE_UI_ID)
                {
                    state.UI.active_element = NONE_UI_ID;
                }
                state.UI.mouse_pressed_on_ui = false;
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                int mouse_x = event.motion.x;
                int mouse_y = event.motion.y;

                if (state.UI.active_element >= SLIDER_RED_UI_ID && state.UI.active_element <= SLIDER_BLUE_UI_ID)
                {
                    int slider_x_position = mouse_x;
                    if (slider_x_position < Red_Slider_Rect.x)
                    {
                        slider_x_position = Red_Slider_Rect.x;
                    }
                    else if (slider_x_position > Red_Slider_Rect.x+Red_Slider_Rect.w)
                    {
                        slider_x_position = Red_Slider_Rect.x + Red_Slider_Rect.w;
                    }
                    float convertor = float( (slider_x_position - Red_Slider_Rect.x)/float(Red_Slider_Rect.w) *255);
                    Uint8 value = Uint8 (convertor);

                    if (state.UI.active_element==SLIDER_RED_UI_ID) state.active_brush_color.red = value;
                    else if (state.UI.active_element==SLIDER_GREEN_UI_ID) state.active_brush_color.green = value;
                    else if (state.UI.active_element==SLIDER_BLUE_UI_ID) state.active_brush_color.blue = value;
                }
                else if (state.mouse_pressed_on_canvas == true)
                {
                    Point Cancas_point = mouse_position_to_canvas_convertor(mouse_x,mouse_y);
                    if (state.current_Active_tool == Brush_tool_id)
                        set_pixel_color(state.canvas,Cancas_point.x,Cancas_point.y,state.active_brush_color);
                    else if (state.current_Active_tool == Eraser_tool_id)
                        set_pixel_color(state.canvas,Cancas_point.x,Cancas_point.y,state.background_color);
                }
            }

        }

        Uint32* Pixels;
        int sdl_locker;
        SDL_LockTexture(texture,NULL,(void**)&Pixels,&sdl_locker);
        Canvas_to_Buffer_Convertor(state.canvas,Pixels);
        SDL_UnlockTexture(texture);

        SDL_SetRenderDrawColor(renderer,100,100,100,255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer,180,180,180,255);
        SDL_Rect Left_Toolbar_Rectangle = {0,0,Left_toolbar_width,window_height};
        SDL_RenderFillRect(renderer,&Left_Toolbar_Rectangle);

        SDL_SetRenderDrawColor(renderer,120,120,120,255);
        SDL_RenderFillRect(renderer,&Red_Slider_Rect);
        SDL_RenderFillRect(renderer,&Green_Slider_Rect);
        SDL_RenderFillRect(renderer,&Blue_Slider_Rect);

        int slider_handle_x_position = Red_Slider_Rect.x + (state.active_brush_color.red*Red_Slider_Rect.w /255);
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderDrawLine(renderer,slider_handle_x_position,Red_Slider_Rect.y,slider_handle_x_position,Red_Slider_Rect.y+Red_Slider_Rect.h);

        slider_handle_x_position = Green_Slider_Rect.x + (state.active_brush_color.green*Green_Slider_Rect.w /255);
        SDL_SetRenderDrawColor(renderer,0,255,0,255);
        SDL_RenderDrawLine(renderer,slider_handle_x_position,Green_Slider_Rect.y,slider_handle_x_position,Green_Slider_Rect.y+Green_Slider_Rect.h);

        slider_handle_x_position = Blue_Slider_Rect.x + (state.active_brush_color.blue*Blue_Slider_Rect.w /255);
        SDL_SetRenderDrawColor(renderer,0,0,255,255);
        SDL_RenderDrawLine(renderer,slider_handle_x_position,Blue_Slider_Rect.y,slider_handle_x_position,Blue_Slider_Rect.y+Blue_Slider_Rect.h);

        SDL_SetRenderDrawColor(renderer,state.active_brush_color.red,state.active_brush_color.green,state.active_brush_color.blue,state.active_brush_color.alpha);
        SDL_RenderFillRect(renderer,&Color_Previewer_Rect);

        SDL_Rect* Tool_Buttons[6]={&Brush_Button,&Fill_Bucket_Button,&Eraser_Button,&Line_Draw_Button,&Rectangle_Draw_Button,&Circle_Draw_Button};
        const char* Tool_Names[]={"Brush","Fill","Eraser","Line","Rect","Circle"};
        for (int i = 0; i < 6; ++i)
        {
            SDL_SetRenderDrawColor(renderer,220,220,220,255);
            if (i == state.current_Active_tool)
            {
                SDL_SetRenderDrawColor(renderer,100,220,220,255);
            }
            SDL_RenderFillRect(renderer,Tool_Buttons[i]);

            if (state.font)
            {
                SDL_Color text_color = {0,0,0,255};
                text_render(renderer,state.font,Tool_Names[i],Tool_Buttons[i]->x + (Tool_Buttons[i]->w)/2,Tool_Buttons[i]->y+(Tool_Buttons[i]->h)/2,text_color,true);
            }

        }

        SDL_SetRenderDrawColor(renderer,255,200,200,255);
        SDL_RenderFillRect(renderer,&Reset_Canvas_Button);
        SDL_SetRenderDrawColor(renderer,125,100,100,255);
        SDL_RenderDrawRect(renderer, &Reset_Canvas_Button);

        if (state.font)
        {
            SDL_Color text_color = {0,0,0,255};
            text_render(renderer,state.font,"Reset Canvas",Reset_Canvas_Button.x+Reset_Canvas_Button.w/2,Reset_Canvas_Button.y+Reset_Canvas_Button.h/2,text_color,true);
        }

        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_RenderFillRect(renderer,&File_Name_Inputer);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderDrawRect(renderer,&File_Name_Inputer);
        if (state.font)
        {
            SDL_Color text_color = {0,0,0,255};
            string Displayed_Text = state.UI.filename;
            if (state.UI.Is_text_input_Active == true)
            {
                Displayed_Text = Displayed_Text + "_";
            }
            text_render(renderer,state.font,Displayed_Text.c_str(),File_Name_Inputer.x+4,File_Name_Inputer.y+4,text_color,false);
        }


        SDL_SetRenderDrawColor(renderer,200,200,200,255);
        SDL_RenderFillRect(renderer,&Load_File_Button);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderDrawRect(renderer,&Load_File_Button);
        if (state.font)
        {
            SDL_Color text_color = {0,0,0,255};
            text_render(renderer,state.font,"Load",Load_File_Button.x+Load_File_Button.w/2,Load_File_Button.y+Load_File_Button.h/2,text_color,true);
        }


        SDL_SetRenderDrawColor(renderer,200,200,200,255);
        SDL_RenderFillRect(renderer,&Save_File_Button);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderDrawRect(renderer,&Save_File_Button);
        if (state.font)
        {
            SDL_Color text_color = {0,0,0,255};
            text_render(renderer,state.font,"Save",Save_File_Button.x+Save_File_Button.w/2,Save_File_Button.y+Save_File_Button.h/2,text_color,true);
        }

        SDL_Rect desktop = {Left_toolbar_width,0,canvas_width*scaler,canvas_height*scaler};
        SDL_RenderCopy(renderer,texture,NULL,&desktop);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);

    }











}





















