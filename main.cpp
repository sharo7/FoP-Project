#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <fstream>

#include "interpreter.h"
#include "sprite.h"
#include "motion_commands.h"
#include "looks_commands.h"
#include "sound_commands.h"
#include "sensing_commands.h"
#include "control_flow_logic.h"
#include "events.h"
#include "operators.h"
#include "variables.h"

using namespace std;

const int window_screen_width  = 1600;
const int window_screen_height = 900;
const int shop_screen_width    = 300;
const int placing_screen_width = 500;
const int satge_width          = window_screen_width - shop_screen_width - placing_screen_width;
const int stage_starting_x     = shop_screen_width + placing_screen_width;
const int block_height         = 34;
const int blocks_gap           = 6;
const int snap_area            = 22;
const int input_width          = 46;
const int input_height         = 22;

const int Scripts_Tab=0, Customs_Tab=1, Sounds_Tab=2;
const int Placing_Tab_MOTION=0, Placing_Tab_LOOKS=1, Placing_Tab_SOUND=2, Placing_Tab_CONTROL=3,
          Placing_Tab_SENSING=4, Placing_Tab_OPERATORS=5, Placing_Tab_VARIABLES=6, Placing_Tab_COUNT=7;

struct Blocks_In_IDE {
    BlockType block_type;
    string    label;
    SDL_Color color;
    SDL_Rect  IDE_BLOCK_Rect;
};

struct LabelPart {
    bool   Is_text_an_Input;
    string text;
    int    InputID_X;
};

static int countInputs(const string &label) {
    int n=0; for (char c:label) if (c=='(') n++; return n;
}

const int CONTAINER_INDENT    = 24;
const int CONTAINER_ARM_W     = 14;
const int CONTAINER_BOT_H     = block_height;
const int CONTAINER_MIN_INNER_H = block_height;

static bool isContainerBlock(BlockType b) {
    return (b==BlockType::Repeat||b==BlockType::Forever||b==BlockType::If_Then||
            b==BlockType::If_Then_Else||b==BlockType::RepeatUntil||b==BlockType::WaitUntil);
}
static bool hasElseBranch(BlockType b) { return b==BlockType::If_Then_Else; }

struct ScriptBlock {
    Blocks_In_IDE placed_block_in_canvas;
    int x, y;
    vector<string>   input_Values;
    vector<SDL_Rect> input_Rects;
    int active_Input    = -1;
    int snapParent      = -1;
    int container_Parent = -1;
    int container_Branch = 0;
};

static TTF_Font *FontLarge = nullptr;
static TTF_Font *FontSmall = nullptr;

static int textWidth(TTF_Font *font, const string &s) {
    if (!font||s.empty()) return 0;
    int w=0; TTF_SizeUTF8(font,s.c_str(),&w,nullptr); return w;
}

static void renderText(SDL_Renderer *r, TTF_Font *font, const string &text,
                       int x, int y, SDL_Color color={255,255,255,255}) {
    if (!font||text.empty()) return;
    SDL_Surface *surf=TTF_RenderUTF8_Blended(font,text.c_str(),color);
    if (!surf) return;
    SDL_Texture *tex=SDL_CreateTextureFromSurface(r,surf);
    SDL_Rect dst={x,y,surf->w,surf->h};
    SDL_RenderCopy(r,tex,nullptr,&dst);
    SDL_DestroyTexture(tex); SDL_FreeSurface(surf);
}

static void renderTextCentered(SDL_Renderer *r, TTF_Font *font, const string &text,
                                SDL_Rect area, SDL_Color color={255,255,255,255}) {
    if (!font||text.empty()) return;
    SDL_Surface *surf=TTF_RenderUTF8_Blended(font,text.c_str(),color);
    if (!surf) return;
    SDL_Texture *tex=SDL_CreateTextureFromSurface(r,surf);
    SDL_Rect dst={area.x+(area.w-surf->w)/2, area.y+(area.h-surf->h)/2, surf->w, surf->h};
    SDL_RenderCopy(r,tex,nullptr,&dst);
    SDL_DestroyTexture(tex); SDL_FreeSurface(surf);
}

static void drawRect(SDL_Renderer *r, const SDL_Rect &rect, SDL_Color color) {
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    SDL_RenderFillRect(r,&rect);
}

static vector<vector<Blocks_In_IDE>> buildPalette() {
    vector<vector<Blocks_In_IDE>> tabs(Placing_Tab_COUNT);

    SDL_Color mc={70,130,180,255};
    tabs[Placing_Tab_MOTION]={
        {BlockType::Move,               "Move (  ) steps",              mc,{}},
        {BlockType::TurnClockwise,      "Turn ClockWise (  )",           mc,{}},
        {BlockType::TurnAnticlockwise,  "Turn CounterClockWise (  )",    mc,{}},
        {BlockType::PointInDirection,   "Point direction (  )",          mc,{}},
        {BlockType::GoToXY,             "Go to X:(  ) Y:(  )",           mc,{}},
        {BlockType::ChangeXBy,          "Change X by (  )",              mc,{}},
        {BlockType::ChangeYBy,          "Change Y by (  )",              mc,{}},
        {BlockType::GoToMousePointer,   "Go to mouse pointer location",  mc,{}},
        {BlockType::GoToRandomPosition, "Go to random position",         mc,{}},
        {BlockType::IfOnEdgeBounce,     "bounce if coliding with the edge", mc,{}},
    };
    SDL_Color lc={148,103,189,255};
    tabs[Placing_Tab_LOOKS]={
        {BlockType::Show,               "Show",                     lc,{}},
        {BlockType::Hide,               "Hide",                     lc,{}},
        {BlockType::NextCostume,        "Next costume",             lc,{}},
        {BlockType::SetSizeTo,          "Set size to (  )%",        lc,{}},
        {BlockType::ChangeSizeBy,       "Change size by (  )",      lc,{}},
        {BlockType::ClearGraphicEffects,"Clear graphic effects",    lc,{}},
        {BlockType::SetColorEffectTo,   "Set color effect (  )",    lc,{}},
        {BlockType::ChangeColorEffectBy,"Change color effect (  )", lc,{}},
        {BlockType::NextBackdrop,       "Next backdrop",            lc,{}},
        {BlockType::Say,                "Say (  )",                 lc,{}},
        {BlockType::Think,              "Think (  )",               lc,{}},
        {BlockType::SayForSeconds,      "Say (  ) for (  ) secs",  lc,{}},
        {BlockType::ThinkForSeconds,    "Think (  ) for (  ) secs",lc,{}},
    };
    SDL_Color sc={220,105,132,255};
    tabs[Placing_Tab_SOUND]={
        {BlockType::StartSound,         "Start sound (name)",      sc,{}},
        {BlockType::PlaySoundUntilDone, "Play sound until done",   sc,{}},
        {BlockType::StopAllSounds,      "Stop all sounds",         sc,{}},
        {BlockType::SetVolumeTo,        "Set volume to (  )%",     sc,{}},
        {BlockType::ChangeVolumeBy,     "Change volume by (  )",   sc,{}},
    };
    SDL_Color cc={255,171,25,255};
    tabs[Placing_Tab_CONTROL]={
        {BlockType::WhenGreenFlagClicked,"When green_button clicked",{50,180,50,255},{}},
        {BlockType::Wait,               "Wait (  ) seconds",      cc,{}},
        {BlockType::Repeat,             "Repeat (  )",             cc,{}},
        {BlockType::Forever,            "Forever",                 cc,{}},
        {BlockType::If_Then,            "If (  ) then",            cc,{}},
        {BlockType::If_Then_Else,       "If (  ) then else",       cc,{}},
        {BlockType::WaitUntil,          "Wait until (  )",         cc,{}},
        {BlockType::RepeatUntil,        "Repeat until (  )",       cc,{}},
        {BlockType::StopAll,            "Stop all",                cc,{}},
    };
    SDL_Color sec={92,177,214,255};
    tabs[Placing_Tab_SENSING]={
        {BlockType::TouchingMousePointer,"Touching mouse?",   sec,{}},
        {BlockType::TouchingEdge,        "Touching edge?",    sec,{}},
        {BlockType::TouchingSprite,      "Touching sprite?",  sec,{}},
        {BlockType::DistanceToMouse,     "Distance to mouse", sec,{}},
        {BlockType::DistanceToSprite,    "Distance to sprite",sec,{}},
        {BlockType::MouseX,              "Mouse X",           sec,{}},
        {BlockType::MouseY,              "Mouse Y",           sec,{}},
        {BlockType::MouseDown,           "Mouse down?",       sec,{}},
        {BlockType::KeyPressed,          "Key (  ) pressed?", sec,{}},
        {BlockType::Timer,               "Timer",             sec,{}},
        {BlockType::ResetTimer,          "Reset timer",       sec,{}},
    };
    SDL_Color oc={89,189,89,255};
    tabs[Placing_Tab_OPERATORS]={
        {BlockType::Addition,       "(a) + (b)",       oc,{}},
        {BlockType::Subtraction,    "(a) - (b)",       oc,{}},
        {BlockType::Multiplication, "(a) * (b)",       oc,{}},
        {BlockType::Division,       "(a) / (b)",       oc,{}},
        {BlockType::Modulos,        "(a) mod (b)",     oc,{}},
        {BlockType::IsEqual,        "(a) = (b)",       oc,{}},
        {BlockType::IsGreaterThan,  "(a) > (b)",       oc,{}},
        {BlockType::IsLessThan,     "(a) < (b)",       oc,{}},
        {BlockType::MyAbs,          "abs (  )",        oc,{}},
        {BlockType::MySqrt,         "sqrt (  )",       oc,{}},
        {BlockType::MyAnd,          "(a) and (b)",     oc,{}},
        {BlockType::MyOr,           "(a) or (b)",      oc,{}},
        {BlockType::MyNot,          "not (a)",         oc,{}},
        {BlockType::LengthOfString, "length of (str)", oc,{}},
        {BlockType::StringConcat,   "join (s1) (s2)",  oc,{}},
    };
    SDL_Color vc={255,140,0,255};
    tabs[Placing_Tab_VARIABLES]={
        {BlockType::CreateVariable,      "Create variable (name)", vc,{}},
        {BlockType::SetVariableValue,    "Set (var) to (val)",     vc,{}},
        {BlockType::ChangeVariableValue, "Change (var) by (val)",  vc,{}},
        {BlockType::ShowVariable,        "Show variable (name)",   vc,{}},
        {BlockType::HideVariable,        "Hide variable (name)",   vc,{}},
        {BlockType::GetVariableValue,    "Get variable (name)",    vc,{}},
    };
    return tabs;
}

static const char *placing_tabs_names[Placing_Tab_COUNT] = {
    "Motion","Looks","Sound","Control","Sensing","Operators","Vars"};
static SDL_Color placing_tab_colors[Placing_Tab_COUNT] = {
    {70,130,180,255},{148,103,189,255},{220,105,132,255},
    {255,171,25,255},{92,177,214,255},{89,189,89,255},{255,140,0,255}};

struct UIRects {
    SDL_Rect Placing_ettePanel, Placing_BlockArea, Placing_TabButtons[Placing_Tab_COUNT];
    SDL_Rect middlePanel, scriptCanvas, scriptButton, costumeButton, soundButton;
    SDL_Rect stagePanel, stageView, greenFlagButton, stopButton;
    SDL_Rect spriteListPanel, addSpriteButton;
};

static UIRects buildRects() {
    UIRects ui;
    ui.Placing_ettePanel = {0,0,shop_screen_width,window_screen_height};
    int tbw=shop_screen_width/2, tbh=40;
    for (int i=0;i<Placing_Tab_COUNT;i++) ui.Placing_TabButtons[i]={(i%2)*tbw,(i/2)*tbh,tbw,tbh};
    int tabBarH=((Placing_Tab_COUNT+1)/2)*tbh;
    ui.Placing_BlockArea={0,tabBarH,shop_screen_width,window_screen_height-tabBarH};
    ui.middlePanel={shop_screen_width,0,placing_screen_width,window_screen_height};
    int pw=placing_screen_width/3;
    ui.scriptButton ={shop_screen_width,0,pw,50};
    ui.costumeButton={shop_screen_width+pw,0,pw,50};
    ui.soundButton  ={shop_screen_width+2*pw,0,pw,50};
    ui.scriptCanvas ={shop_screen_width,50,placing_screen_width,window_screen_height-50};
    ui.stagePanel      ={stage_starting_x,0,satge_width,window_screen_height};
    ui.greenFlagButton ={stage_starting_x+10,10,40,40};
    ui.stopButton      ={stage_starting_x+60,10,40,40};
    ui.stageView       ={stage_starting_x,60,satge_width,window_screen_height-160};
    ui.spriteListPanel ={stage_starting_x,window_screen_height-100,satge_width,100};
    ui.addSpriteButton ={stage_starting_x+10,window_screen_height-90,80,40};
    return ui;
}

struct AppState {
    int  Placing_Tab    = Placing_Tab_MOTION;
    int  placeTab       = Scripts_Tab;
    bool running        = true;
    int  Placing_ScrollY = 0;
    int  mouseX=0, mouseY=0;
};

int main() {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0){cout<<SDL_GetError();return 1;}
    if (TTF_Init()<0){cout<<TTF_GetError();return 1;}
    if (!IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG)){cout<<IMG_GetError();return 1;}

    SDL_Window *window=SDL_CreateWindow("Scratch IDE",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        window_screen_width,window_screen_height,SDL_WINDOW_SHOWN);
    if (!window){cout<<SDL_GetError();return 1;}

    SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){cout<<SDL_GetError();return 1;}

    FontLarge=TTF_OpenFont("./Fonts/arial.ttf",17);
    FontSmall=TTF_OpenFont("./Fonts/arial.ttf",13);
    if (!FontLarge||!FontSmall) cout<<"[warn] font not loaded\n";

    auto palette=buildPalette();
    UIRects ui=buildRects();
    AppState state;

    SDL_Event sdlEvent;
    while (state.running) {
        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_QUIT: state.running=false; break;
                case SDL_MOUSEMOTION:
                    state.mouseX=sdlEvent.motion.x;
                    state.mouseY=sdlEvent.motion.y;
                    break;
                case SDL_MOUSEWHEEL: {
                    SDL_Point mp={state.mouseX,state.mouseY};
                    if (SDL_PointInRect(&mp,&ui.Placing_BlockArea)) {
                        state.Placing_ScrollY-=sdlEvent.wheel.y*(block_height+blocks_gap);
                        if (state.Placing_ScrollY<0) state.Placing_ScrollY=0;
                    }
                    break;
                }
                default: break;
            }
            if (sdlEvent.type==SDL_MOUSEBUTTONDOWN && sdlEvent.button.button==SDL_BUTTON_LEFT) {
                SDL_Point mp={sdlEvent.button.x,sdlEvent.button.y};
                for (int i=0;i<Placing_Tab_COUNT;i++)
                    if (SDL_PointInRect(&mp,&ui.Placing_TabButtons[i])){
                        state.Placing_Tab=i; state.Placing_ScrollY=0; }
                if (SDL_PointInRect(&mp,&ui.scriptButton))  state.placeTab=Scripts_Tab;
                if (SDL_PointInRect(&mp,&ui.costumeButton)) state.placeTab=Customs_Tab;
                if (SDL_PointInRect(&mp,&ui.soundButton))   state.placeTab=Sounds_Tab;
            }
        }

        SDL_SetRenderDrawColor(renderer,30,30,30,255);
        SDL_RenderClear(renderer);

        {
            SDL_SetRenderDrawColor(renderer,45,45,55,255);
            SDL_RenderFillRect(renderer,&ui.Placing_ettePanel);
            for (int i=0;i<Placing_Tab_COUNT;i++) {
                SDL_Color tc=placing_tab_colors[i];
                if (i!=state.Placing_Tab){tc.r/=2;tc.g/=2;tc.b/=2;}
                drawRect(renderer,ui.Placing_TabButtons[i],tc);
                renderTextCentered(renderer,FontSmall,placing_tabs_names[i],ui.Placing_TabButtons[i]);
                SDL_SetRenderDrawColor(renderer,20,20,20,255);
                SDL_Rect sep={ui.Placing_TabButtons[i].x,
                              ui.Placing_TabButtons[i].y+ui.Placing_TabButtons[i].h-1,
                              ui.Placing_TabButtons[i].w,1};
                SDL_RenderFillRect(renderer,&sep);
            }
            SDL_RenderSetClipRect(renderer,&ui.Placing_BlockArea);
            auto &cat=palette[state.Placing_Tab];
            SDL_Color pc=placing_tab_colors[state.Placing_Tab];
            for (int i=0;i<(int)cat.size();i++) {
                SDL_Rect r={ui.Placing_BlockArea.x+8,
                            ui.Placing_BlockArea.y+8+i*(block_height+blocks_gap)-state.Placing_ScrollY,
                            ui.Placing_BlockArea.w-16,block_height};
                if (r.y+r.h<ui.Placing_BlockArea.y) continue;
                if (r.y>ui.Placing_BlockArea.y+ui.Placing_BlockArea.h) break;
                drawRect(renderer,r,pc);
                renderText(renderer,FontSmall,cat[i].label,r.x+6,r.y+(block_height-13)/2);
            }
            SDL_RenderSetClipRect(renderer,nullptr);
        }

        {
            SDL_SetRenderDrawColor(renderer,55,55,65,255);
            SDL_RenderFillRect(renderer,&ui.middlePanel);
            auto tabBtn=[&](SDL_Rect b,const char *lbl,int t){
                drawRect(renderer,b,state.placeTab==t?SDL_Color{80,120,200,255}:SDL_Color{60,60,70,255});
                renderTextCentered(renderer,FontSmall,lbl,b);
            };
            tabBtn(ui.scriptButton,"Scripts",Scripts_Tab);
            tabBtn(ui.costumeButton,"Costumes",Customs_Tab);
            tabBtn(ui.soundButton,"Sounds",Sounds_Tab);
            SDL_SetRenderDrawColor(renderer,40,40,50,255);
            SDL_RenderFillRect(renderer,&ui.scriptCanvas);
            renderTextCentered(renderer,FontSmall,"Drag blocks here",ui.scriptCanvas,{80,80,100,255});
        }

        {
            SDL_SetRenderDrawColor(renderer,35,35,40,255);
            SDL_RenderFillRect(renderer,&ui.stagePanel);
            drawRect(renderer,ui.greenFlagButton,{30,140,50,255});
            renderTextCentered(renderer,FontLarge,">",ui.greenFlagButton);
            drawRect(renderer,ui.stopButton,{180,50,50,255});
            renderTextCentered(renderer,FontLarge,"■",ui.stopButton);
            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderFillRect(renderer,&ui.stageView);
            SDL_SetRenderDrawColor(renderer,45,45,55,255);
            SDL_RenderFillRect(renderer,&ui.spriteListPanel);
            drawRect(renderer,ui.addSpriteButton,{70,130,200,255});
            renderTextCentered(renderer,FontSmall,"+ Sprite",ui.addSpriteButton);
        }

        SDL_SetRenderDrawColor(renderer,15,15,15,255);
        SDL_Rect ld={shop_screen_width-2,0,2,window_screen_height};
        SDL_Rect md={shop_screen_width+placing_screen_width-2,0,2,window_screen_height};
        SDL_RenderFillRect(renderer,&ld);
        SDL_RenderFillRect(renderer,&md);

        SDL_RenderPresent(renderer);
    }

    if (FontSmall) TTF_CloseFont(FontSmall);
    if (FontLarge) TTF_CloseFont(FontLarge);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit(); TTF_Quit(); SDL_Quit();
    return 0;
}