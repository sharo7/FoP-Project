#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

const int window_width = 1600 , window_height = 900;
const int block_shop_width = 300;
const int block_place_width = 500;
const int sprite_tab = window_width - block_place_width - block_shop_width;

const int number_of_blocks_to_show_in_tab = 10;

struct UI_State
{
    bool mouse_pressed_on_blocks_shop;
    bool mouse_pressed_on_a_block_on_placing_tab;
    int active_element;
};

struct Sound
{
    string sound_name;
    string sound_path;
};

struct costumes
{
    vector<string> custome_names;
    int number_of_costumes;
    vector<string> custom_loactions;
    vector<Sound> custome_sounds;
};


struct Color
{
    Uint8 red, green, blue, alpha;
};


struct Block
{
    int Type_ID;
    Color block_color;
    int block_width;
    string label;
    SDL_Rect block_rect;

    bool has_input = false;
    int number_of_inputs;
    vector<int> input_locations;
    vector<string> input_values;
    Block* previous = nullptr;
    Block* next = nullptr;

    bool has_dropdown1 =false;
    bool has_dropdown2 =false;
    int dropdown1_location;
    int dropdown2_location;
};

struct IDE_Active_Block
{
    Block current_Block;
    Block* next_Block = nullptr;
    Block* previous_Block = nullptr;
    int loacation_x;
    int loacation_y;
};

struct saved_blocks
{
    int i;
    int block_type_id;
    SDL_Rect block_rect;
};

struct App_State
{
    int current_active_blocks_tab;
    int current_active_placing_tab;
    int current_active_sprite;
    UI_State UI_state;
};



const int Motion_blocks = 0;
vector<Block> Motion_Blocks;
const int Control_blocks = 1;
vector<Block> Control_Blocks;
const int Looks_blocks = 2;
vector<Block> Looks_Blocks;
const int Sensing_blocks = 3;
vector<Block> Sensing_Blocks;
const int Sound_blocks = 4;
vector<Block> Sound_Blocks;
const int Operators_blocks = 5;
vector<Block> Operators_Blocks;
const int Variable_blocks = 6;
vector<Block> Variable_Blocks;
vector<vector<Block>> Blocks_to_choose =
{
    Motion_Blocks,
    Control_Blocks,
    Looks_Blocks,
    Sensing_Blocks,
    Sound_Blocks,
    Operators_Blocks,
    Variable_Blocks
};



const int scripts_tab = 0;
const int costumes_tab = 1;
const int sounds_tab = 2;





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


int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        cout << "SDL_Failed: " << SDL_GetError() << endl;
        return -1;
    }
    if (TTF_Init() < 0)
    {
        cout<< "TTF_SDL failed: " << TTF_GetError() << endl;
        SDL_Quit();
        return -1;
    }


    SDL_Window* window = SDL_CreateWindow("sprite_editor",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,window_width,window_height,SDL_WINDOW_SHOWN);

    if (!window)
    {
        cout<< "SDL_CreateWindow failed: " << SDL_GetError() << endl;
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!renderer)
    {
        cout<< "SDL_CreateRenderer failed: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    SDL_PixelFormat* RGBA_Format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);

    TTF_Font* font = TTF_OpenFont("./Fonts/arial.ttf",16);
    if (!font)
    {
        cout<< "TTF_OpenFont failed: " << TTF_GetError() << endl;
        cout<<"no text is can be visable"<<endl;
    }

    App_State state;
    vector<SDL_Rect> Shop_Category_buttons;
    SDL_Rect Shop_Category_Background = {10,80,block_shop_width+20,window_height};
    SDL_Rect Blocks_Category_Background = {20,80,block_shop_width,220};
    SDL_Rect Blocks_Shop_Background = {20,320,block_shop_width,window_height-320};
    SDL_Rect Motion_Blocks_Button = {40,100,110,40};
    Shop_Category_buttons.push_back(Motion_Blocks_Button);
    SDL_Rect Looks_Blocks_Button = {40,150,110,40};
    Shop_Category_buttons.push_back(Looks_Blocks_Button);
    SDL_Rect Sound_Blocks_Button = {40,200,110,40};
    Shop_Category_buttons.push_back(Sound_Blocks_Button);
    SDL_Rect Variable_Blocks_Button = {40,250,110,40};
    Shop_Category_buttons.push_back(Variable_Blocks_Button);
    SDL_Rect Control_Blocks_Button = {170,100,110,40};
    Shop_Category_buttons.push_back(Control_Blocks_Button);
    SDL_Rect Sensing_Blocks_Button = {170,150,110,40};
    Shop_Category_buttons.push_back(Sensing_Blocks_Button);
    SDL_Rect Operators_Blocks_Button = {170,200,110,40};
    Shop_Category_buttons.push_back(Operators_Blocks_Button);

    SDL_Rect Placing_Background = {block_shop_width+40,80,block_place_width+20,window_height};
    SDL_Rect Placing_Category_Background = {350,80,block_place_width,180};
    SDL_Rect Placing_Location_Background = {350,280,block_place_width,window_height-280};
    SDL_Rect Scripts_Tab_Switch_button = {390, 230,80,30};
    SDL_Rect Costumes_Tab_Switch_button = {490, 230,80,30};
    SDL_Rect Sound_Tab_Switch_button = {590, 230,80,30};
    state.current_active_blocks_tab=0;
    state.current_active_placing_tab=0;

    vector<IDE_Active_Block> Blocks_in_ide_area;
    vector<costumes> current_costumes;

    bool Is_Running = true;
    int current_block_number_to_show_start =0;
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


            }
            else if (event.type==SDL_TEXTINPUT)
            {

            }
            else if (event.type==SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {


            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            {

            }
            else if (event.type == SDL_MOUSEMOTION)
            {

            }

        }

        vector<saved_blocks> saved_blocks_inthisinstance;

        SDL_SetRenderDrawColor(renderer,150,150,150,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,120,120,120,255);
        SDL_RenderFillRect(renderer,&Shop_Category_Background);
        SDL_RenderFillRect(renderer,&Placing_Background);
        SDL_SetRenderDrawColor(renderer,100,100,100,255);
        SDL_RenderFillRect(renderer,&Blocks_Category_Background);
        SDL_RenderFillRect(renderer,&Placing_Category_Background);
        SDL_RenderFillRect(renderer,&Blocks_Shop_Background);
        SDL_RenderFillRect(renderer,&Placing_Location_Background);
        SDL_SetRenderDrawColor(renderer,80,80,80,255);
        for (int i=0;i<Shop_Category_buttons.size();i++)
            SDL_RenderFillRect(renderer,&Shop_Category_buttons.at(i));
        SDL_RenderFillRect(renderer,&Scripts_Tab_Switch_button);
        SDL_RenderFillRect(renderer,&Costumes_Tab_Switch_button);
        SDL_RenderFillRect(renderer,&Sound_Tab_Switch_button);

        for (int i=0;i<number_of_blocks_to_show_in_tab and !Blocks_to_choose.at(state.current_active_blocks_tab).empty() ;i++)
        {
            SDL_Rect new_block = {40,360+i*40,Blocks_to_choose.at(state.current_active_blocks_tab).at(i).block_width,30};
            SDL_SetRenderDrawColor(renderer,Blocks_to_choose.at(state.current_active_blocks_tab).at(i).block_color.red,Blocks_to_choose.at(state.current_active_blocks_tab).at(i).block_color.green,Blocks_to_choose.at(state.current_active_blocks_tab).at(i).block_color.blue,Blocks_to_choose.at(state.current_active_blocks_tab).at(i).block_color.alpha);
            SDL_RenderFillRect(renderer,&new_block);
            Blocks_to_choose.at(state.current_active_blocks_tab).at(i).block_rect = new_block;

            saved_blocks new_saved_block;

            new_saved_block.i=i;
            new_saved_block.block_rect = new_block;
            new_saved_block.block_type_id=state.current_active_blocks_tab;

            saved_blocks_inthisinstance.push_back(new_saved_block);

            if (Blocks_to_choose.at(state.current_active_blocks_tab).at(i).has_input)
            {
                SDL_SetRenderDrawColor(renderer,255,255,255,255);
                for (int j=0; j<Blocks_to_choose.at(state.current_active_blocks_tab).at(i).input_locations.at(j);j++)
                    {
                        SDL_Rect text_box = {40+Blocks_to_choose.at(state.current_active_blocks_tab).at(i).input_locations.at(j),360+i*40+5,20,20};
                        SDL_RenderFillRect(renderer,&text_box);
                    }
                SDL_SetRenderDrawColor(renderer,100,100,100,255);
            }
            if (Blocks_to_choose.at(state.current_active_blocks_tab).at(i).has_dropdown1)
            {
                SDL_Rect dropdown_1 = {40+Blocks_to_choose.at(state.current_active_blocks_tab).at(i).dropdown1_location};
                SDL_RenderFillRect(renderer,&dropdown_1);
            }
            if (Blocks_to_choose.at(state.current_active_blocks_tab).at(i).has_dropdown2)
            {
                SDL_Rect dropdown_2 = {40+Blocks_to_choose.at(state.current_active_blocks_tab).at(i).dropdown2_location};
                SDL_RenderFillRect(renderer,&dropdown_2);
            }
        }

        if (state.current_active_placing_tab == scripts_tab)
        {
            for (int j=0;j<Blocks_in_ide_area.size();j++)
            {
                SDL_Rect ide_block ={Blocks_in_ide_area.at(j).loacation_x,Blocks_in_ide_area.at(j).loacation_y,Blocks_in_ide_area.at(j).current_Block.block_width,30};
                SDL_SetRenderDrawColor(renderer,Blocks_in_ide_area.at(j).current_Block.block_color.red,Blocks_in_ide_area.at(j).current_Block.block_color.green,Blocks_in_ide_area.at(j).current_Block.block_color.blue,Blocks_in_ide_area.at(j).current_Block.block_color.alpha);
                SDL_RenderFillRect(renderer,&ide_block);
                if (Blocks_in_ide_area.at(j).current_Block.has_input)
                {
                    SDL_SetRenderDrawColor(renderer,255,255,255,255);
                    for (int i=0; i<Blocks_in_ide_area.at(j).current_Block.input_locations.size();i++)
                    {
                        SDL_Rect text_box = {Blocks_in_ide_area.at(j).loacation_x+Blocks_in_ide_area.at(j).current_Block.input_locations.at(i),Blocks_in_ide_area.at(j).loacation_y+5,20,20};
                        SDL_RenderFillRect(renderer,&text_box);
                    }
                    SDL_SetRenderDrawColor(renderer,100,100,100,255);
                }
                if (Blocks_in_ide_area.at(j).current_Block.has_dropdown1)
                {
                    SDL_Rect dropdown_1 = {Blocks_in_ide_area.at(j).loacation_x+Blocks_in_ide_area.at(j).current_Block.dropdown1_location,Blocks_in_ide_area.at(j).loacation_y+5,20,20};
                    SDL_RenderFillRect(renderer,&dropdown_1);
                }
                if (Blocks_in_ide_area.at(j).current_Block.has_dropdown2)
                {
                    SDL_Rect dropdown_2 = {Blocks_in_ide_area.at(j).loacation_x+Blocks_in_ide_area.at(j).current_Block.dropdown2_location,Blocks_in_ide_area.at(j).loacation_y+5,20,20};
                    SDL_RenderFillRect(renderer,&dropdown_2);
                }
            }
        }
        else if (state.current_active_placing_tab == costumes_tab)
        {
            for (int i=0;i<current_costumes.at(state.current_active_sprite).number_of_costumes;i++)
            {
                ////////draw custome shananigans
            }
        }
        else if (state.current_active_blocks_tab == sounds_tab)
        {
            for (int i=0;i<current_costumes.at(state.current_active_sprite).custome_sounds.size();i++)
            {
                ////draw sound for active sprite
            }
        }










        SDL_RenderPresent(renderer);
        SDL_Delay(16);

    }
}