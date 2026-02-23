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

static vector<LabelPart> parseLabel(const string &label) {
    vector<LabelPart> parts;
    int inputCount=0;
    size_t i=0;
    string buf;
    while (i<label.size()) {
        if (label[i]=='(') {
            if (!buf.empty()){parts.push_back({false,buf,-1});buf.clear();}
            size_t j=label.find(')',i);
            if (j==string::npos) j=label.size()-1;
            parts.push_back({true,label.substr(i+1,j-i-1),inputCount++});
            i=j+1;
        } else { buf+=label[i++]; }
    }
    if (!buf.empty()) parts.push_back({false,buf,-1});
    return parts;
}

static int countInputs(const string &label) {
    int n=0; for (char c:label) if (c=='(') n++; return n;
}

const int CONTAINER_INDENT     = 24;
const int CONTAINER_ARM_W      = 14;
const int CONTAINER_BOT_H      = block_height;
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
    int active_Input     = -1;
    int snapParent       = -1;
    int container_Parent = -1;
    int container_Branch = 0;
};

static int blockTotalHeight(const vector<ScriptBlock> &blocks, int ID_X);

static int blockTotalHeight(const vector<ScriptBlock> &blocks, int ID_X) {
    const auto &sb=blocks[ID_X];
    if (!isContainerBlock(sb.placed_block_in_canvas.block_type)) return block_height;

    int h1=0;
    for (int i=0;i<(int)blocks.size();i++) {
        if (blocks[i].container_Parent==ID_X&&blocks[i].container_Branch==0&&blocks[i].snapParent==-1) {
            int cur=i;
            while (cur!=-1) {
                h1+=blockTotalHeight(blocks,cur);
                int nxt=-1;
                for (int k=0;k<(int)blocks.size();k++)
                    if (blocks[k].snapParent==cur&&blocks[k].container_Parent==ID_X&&blocks[k].container_Branch==0){nxt=k;break;}
                cur=nxt;
            }
        }
    }
    if (h1<CONTAINER_MIN_INNER_H) h1=CONTAINER_MIN_INNER_H;
    if (!hasElseBranch(sb.placed_block_in_canvas.block_type))
        return block_height+h1+CONTAINER_BOT_H;

    int h2=0;
    for (int i=0;i<(int)blocks.size();i++) {
        if (blocks[i].container_Parent==ID_X&&blocks[i].container_Branch==1&&blocks[i].snapParent==-1) {
            int cur=i;
            while (cur!=-1) {
                h2+=blockTotalHeight(blocks,cur);
                int nxt=-1;
                for (int k=0;k<(int)blocks.size();k++)
                    if (blocks[k].snapParent==cur&&blocks[k].container_Parent==ID_X&&blocks[k].container_Branch==1){nxt=k;break;}
                cur=nxt;
            }
        }
    }
    if (h2<CONTAINER_MIN_INNER_H) h2=CONTAINER_MIN_INNER_H;
    return block_height+h1+CONTAINER_BOT_H+h2+CONTAINER_BOT_H;
}

static TTF_Font *FontLarge = nullptr;
static TTF_Font *FontSmall = nullptr;
static SDL_Texture *SayBubbleTex   = nullptr;
static SDL_Texture *ThinkBubbleTex = nullptr;

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
    SDL_Rect dst={area.x+(area.w-surf->w)/2,area.y+(area.h-surf->h)/2,surf->w,surf->h};
    SDL_RenderCopy(r,tex,nullptr,&dst);
    SDL_DestroyTexture(tex); SDL_FreeSurface(surf);
}

static void drawRect(SDL_Renderer *r, const SDL_Rect &rect, SDL_Color color) {
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    SDL_RenderFillRect(r,&rect);
}

static int renderScriptBlock(SDL_Renderer *renderer, ScriptBlock &cb,
                              const vector<ScriptBlock> &all, int myIdx, bool ghost=false) {
    auto parts=parseLabel(cb.placed_block_in_canvas.label);
    int totalW=12;
    for (auto &p:parts)
        totalW+=p.Is_text_an_Input ? input_width+4 : textWidth(FontSmall,p.text)+4;
    if (totalW<130) totalW=130;

    SDL_Color bc=cb.placed_block_in_canvas.color;
    if (ghost){bc.a=190;SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);}
    bool isCon=isContainerBlock(cb.placed_block_in_canvas.block_type);

    if (!isCon) {
        SDL_Rect bg={cb.x,cb.y,totalW,block_height};
        SDL_SetRenderDrawColor(renderer,bc.r,bc.g,bc.b,bc.a);
        SDL_RenderFillRect(renderer,&bg);
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer,0,0,0,70);
        SDL_Rect notch={cb.x+14,cb.y+block_height-3,16,3};
        SDL_Rect tnotch={cb.x+14,cb.y,16,3};
        SDL_RenderFillRect(renderer,&notch);
        SDL_RenderFillRect(renderer,&tnotch);
    } else {
        int th=blockTotalHeight(all,myIdx);
        bool hasElse=hasElseBranch(cb.placed_block_in_canvas.block_type);
        int innerH0=0;
        for (int j=0;j<(int)all.size();j++) {
            if (all[j].container_Parent==myIdx&&all[j].container_Branch==0&&all[j].snapParent==-1) {
                for (int k=j;k!=-1;) {
                    innerH0+=blockTotalHeight(all,k);
                    int nxt=-1;
                    for (int m=0;m<(int)all.size();m++)
                        if (all[m].snapParent==k&&all[m].container_Parent==myIdx&&all[m].container_Branch==0){nxt=m;break;}
                    k=nxt;
                }
            }
        }
        if (innerH0<CONTAINER_MIN_INNER_H) innerH0=CONTAINER_MIN_INNER_H;
        SDL_SetRenderDrawColor(renderer,bc.r,bc.g,bc.b,bc.a);
        SDL_Rect header={cb.x,cb.y,totalW,block_height};
        SDL_RenderFillRect(renderer,&header);
        SDL_Rect arm={cb.x,cb.y,CONTAINER_ARM_W,th};
        SDL_RenderFillRect(renderer,&arm);
        int capY0=cb.y+block_height+innerH0;
        SDL_Rect cap0={cb.x,capY0,totalW,CONTAINER_BOT_H};
        SDL_RenderFillRect(renderer,&cap0);
        if (hasElse) {
            renderText(renderer,FontSmall,"else",cb.x+CONTAINER_ARM_W+6,capY0+(CONTAINER_BOT_H-13)/2);
            int innerH1=th-block_height-innerH0-CONTAINER_BOT_H*2;
            if (innerH1<CONTAINER_MIN_INNER_H) innerH1=CONTAINER_MIN_INNER_H;
            int capY1=capY0+CONTAINER_BOT_H+innerH1;
            SDL_Rect cap1={cb.x,capY1,totalW,CONTAINER_BOT_H};
            SDL_RenderFillRect(renderer,&cap1);
        }
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer,0,0,0,70);
        SDL_Rect tnotch={cb.x+14,cb.y,16,3};
        SDL_RenderFillRect(renderer,&tnotch);
        SDL_Rect inotch={cb.x+CONTAINER_ARM_W+8,cb.y+block_height-3,16,3};
        SDL_RenderFillRect(renderer,&inotch);
        SDL_Rect bnotch={cb.x+14,cb.y+th-3,16,3};
        SDL_RenderFillRect(renderer,&bnotch);
    }

    cb.input_Rects.resize(cb.input_Values.size());
    int curX=isCon ? cb.x+CONTAINER_ARM_W+6 : cb.x+6;
    int midY=cb.y+(block_height-13)/2;
    for (auto &p:parts) {
        if (!p.Is_text_an_Input) {
            renderText(renderer,FontSmall,p.text,curX,midY);
            curX+=textWidth(FontSmall,p.text)+4;
        } else {
            bool active=(cb.active_Input==p.InputID_X);
            SDL_Rect ibox={curX,cb.y+(block_height-input_height)/2,input_width,input_height};
            if (p.InputID_X<(int)cb.input_Rects.size()) cb.input_Rects[p.InputID_X]=ibox;
            SDL_Color ibg=active?SDL_Color{255,255,180,255}:SDL_Color{255,255,255,240};
            drawRect(renderer,ibox,ibg);
            SDL_SetRenderDrawColor(renderer,80,80,80,255);
            SDL_RenderDrawRect(renderer,&ibox);
            const string &val=(p.InputID_X<(int)cb.input_Values.size())?cb.input_Values[p.InputID_X]:"";
            if (val.empty()) renderText(renderer,FontSmall,p.text,ibox.x+3,ibox.y+(input_height-12)/2,{160,160,160,255});
            else renderText(renderer,FontSmall,val,ibox.x+3,ibox.y+(input_height-12)/2,{0,0,0,255});
            if (active&&(SDL_GetTicks()/500)%2==0) {
                int cx=ibox.x+3+textWidth(FontSmall,val);
                if (cx>ibox.x+ibox.w-3) cx=ibox.x+ibox.w-3;
                SDL_SetRenderDrawColor(renderer,0,0,0,255);
                SDL_RenderDrawLine(renderer,cx,ibox.y+3,cx,ibox.y+input_height-3);
            }
            curX+=input_width+4;
        }
    }
    return isCon?blockTotalHeight(all,myIdx):block_height;
}

static int containerInnerY(const vector<ScriptBlock> &blocks, int cIdx, int branch=0) {
    const auto &cb=blocks[cIdx];
    if (branch==0) return cb.y+block_height;
    int innerH0=0;
    for (int j=0;j<(int)blocks.size();j++) {
        if (blocks[j].container_Parent==cIdx&&blocks[j].container_Branch==0&&blocks[j].snapParent==-1) {
            for (int k=j;k!=-1;) {
                innerH0+=blockTotalHeight(blocks,k);
                int nxt=-1;
                for (int m=0;m<(int)blocks.size();m++)
                    if (blocks[m].snapParent==k&&blocks[m].container_Parent==cIdx&&blocks[m].container_Branch==0){nxt=m;break;}
                k=nxt;
            }
        }
    }
    if (innerH0<CONTAINER_MIN_INNER_H) innerH0=CONTAINER_MIN_INNER_H;
    return cb.y+block_height+innerH0+CONTAINER_BOT_H;
}

static int lastInBranch(const vector<ScriptBlock> &blocks, int cIdx, int branch) {
    int last=-1;
    for (int j=0;j<(int)blocks.size();j++) {
        if (blocks[j].container_Parent==cIdx&&blocks[j].container_Branch==branch&&blocks[j].snapParent==-1) {
            for (int k=j;k!=-1;) {
                last=k;
                int nxt=-1;
                for (int m=0;m<(int)blocks.size();m++)
                    if (blocks[m].snapParent==k&&blocks[m].container_Parent==cIdx&&blocks[m].container_Branch==branch){nxt=m;break;}
                k=nxt;
            }
        }
    }
    return last;
}

struct SnapResult {
    int  targetIdx      = -1;
    bool intoContainer  = false;
    int  containerIdx   = -1;
    int  containerBranch = 0;
};

static SnapResult findSnapTarget(const vector<ScriptBlock> &blocks, int movingIdx, int mx, int my) {
    SnapResult best;
    for (int i=0;i<(int)blocks.size();i++) {
        if (i==movingIdx) continue;
        if (blocks[i].container_Parent==movingIdx) continue;
        bool isCon=isContainerBlock(blocks[i].placed_block_in_canvas.block_type);
        if (isCon) {
            int totalH=blockTotalHeight(blocks,i);
            if (mx>=blocks[i].x+CONTAINER_ARM_W-10&&mx<=blocks[i].x+200) {
                int b0Top=blocks[i].y+block_height;
                int innerH0=0;
                for (int j=0;j<(int)blocks.size();j++) {
                    if (blocks[j].container_Parent==i&&blocks[j].container_Branch==0&&blocks[j].snapParent==-1) {
                        for (int k=j;k!=-1;) {
                            innerH0+=blockTotalHeight(blocks,k);
                            int nxt=-1;
                            for (int m=0;m<(int)blocks.size();m++)
                                if (blocks[m].snapParent==k&&blocks[m].container_Parent==i&&blocks[m].container_Branch==0){nxt=m;break;}
                            k=nxt;
                        }
                    }
                }
                if (innerH0<CONTAINER_MIN_INNER_H) innerH0=CONTAINER_MIN_INNER_H;
                int b0Bot=b0Top+innerH0;
                if (my>=b0Top-snap_area&&my<=b0Bot+snap_area) {
                    int last0=lastInBranch(blocks,i,0);
                    if (last0>=0) {
                        int botY=blocks[last0].y+blockTotalHeight(blocks,last0);
                        if (abs(my-botY)<=snap_area){best.targetIdx=last0;best.intoContainer=false;return best;}
                    } else {
                        if (abs(my-b0Top)<=snap_area*2){best.intoContainer=true;best.containerIdx=i;best.containerBranch=0;return best;}
                    }
                }
                if (hasElseBranch(blocks[i].placed_block_in_canvas.block_type)) {
                    int b1Top=containerInnerY(blocks,i,1);
                    int innerH1=totalH-block_height-innerH0-CONTAINER_BOT_H*2;
                    if (innerH1<CONTAINER_MIN_INNER_H) innerH1=CONTAINER_MIN_INNER_H;
                    int b1Bot=b1Top+innerH1;
                    if (my>=b1Top-snap_area&&my<=b1Bot+snap_area) {
                        int last1=lastInBranch(blocks,i,1);
                        if (last1>=0) {
                            int botY=blocks[last1].y+blockTotalHeight(blocks,last1);
                            if (abs(my-botY)<=snap_area){best.targetIdx=last1;best.intoContainer=false;return best;}
                        } else {
                            if (abs(my-b1Top)<=snap_area*2){best.intoContainer=true;best.containerIdx=i;best.containerBranch=1;return best;}
                        }
                    }
                }
            }
        }
        int botY=blocks[i].y+blockTotalHeight(blocks,i);
        if (abs(my-botY)<=snap_area&&abs(mx-blocks[i].x)<=40){best.targetIdx=i;best.intoContainer=false;return best;}
    }
    return best;
}

static void propagateContainerChildren(vector<ScriptBlock> &blocks, int cIdx);

static void propagateSnap(vector<ScriptBlock> &blocks, int parentIdx) {
    const auto &pb=blocks[parentIdx];
    bool pIsCon=isContainerBlock(pb.placed_block_in_canvas.block_type);
    int pBotY=pb.y+(pIsCon?blockTotalHeight(blocks,parentIdx):block_height);
    for (int i=0;i<(int)blocks.size();i++) {
        if (blocks[i].snapParent==parentIdx&&blocks[i].container_Parent==-1) {
            blocks[i].x=pb.x; blocks[i].y=pBotY;
            propagateSnap(blocks,i);
        }
    }
    if (pIsCon) propagateContainerChildren(blocks,parentIdx);
}

static void propagateContainerChildren(vector<ScriptBlock> &blocks, int cIdx) {
    auto &cb=blocks[cIdx];
    bool hasElse=hasElseBranch(cb.placed_block_in_canvas.block_type);
    int curY0=cb.y+block_height;
    for (int j=0;j<(int)blocks.size();j++) {
        if (blocks[j].container_Parent==cIdx&&blocks[j].container_Branch==0&&blocks[j].snapParent==-1) {
            blocks[j].x=cb.x+CONTAINER_INDENT; blocks[j].y=curY0;
            int cur=j;
            while (cur!=-1) {
                int nxt=-1;
                for (int m=0;m<(int)blocks.size();m++)
                    if (blocks[m].snapParent==cur&&blocks[m].container_Parent==cIdx&&blocks[m].container_Branch==0){nxt=m;break;}
                if (nxt!=-1){blocks[nxt].x=blocks[cur].x;blocks[nxt].y=blocks[cur].y+blockTotalHeight(blocks,cur);}
                propagateSnap(blocks,cur);
                cur=nxt;
            }
        }
    }
    if (!hasElse) return;
    int innerH0=0;
    for (int j=0;j<(int)blocks.size();j++) {
        if (blocks[j].container_Parent==cIdx&&blocks[j].container_Branch==0&&blocks[j].snapParent==-1) {
            for (int k=j;k!=-1;) {
                innerH0+=blockTotalHeight(blocks,k);
                int nxt=-1;
                for (int m=0;m<(int)blocks.size();m++)
                    if (blocks[m].snapParent==k&&blocks[m].container_Parent==cIdx&&blocks[m].container_Branch==0){nxt=m;break;}
                k=nxt;
            }
        }
    }
    if (innerH0<CONTAINER_MIN_INNER_H) innerH0=CONTAINER_MIN_INNER_H;
    int curY1=cb.y+block_height+innerH0+CONTAINER_BOT_H;
    for (int j=0;j<(int)blocks.size();j++) {
        if (blocks[j].container_Parent==cIdx&&blocks[j].container_Branch==1&&blocks[j].snapParent==-1) {
            blocks[j].x=cb.x+CONTAINER_INDENT; blocks[j].y=curY1;
            int cur=j;
            while (cur!=-1) {
                int nxt=-1;
                for (int m=0;m<(int)blocks.size();m++)
                    if (blocks[m].snapParent==cur&&blocks[m].container_Parent==cIdx&&blocks[m].container_Branch==1){nxt=m;break;}
                if (nxt!=-1){blocks[nxt].x=blocks[cur].x;blocks[nxt].y=blocks[cur].y+blockTotalHeight(blocks,cur);}
                propagateSnap(blocks,cur);
                cur=nxt;
            }
        }
    }
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
        {BlockType::Wait,               "Wait (  ) seconds",  cc,{}},
        {BlockType::Repeat,             "Repeat (  )",         cc,{}},
        {BlockType::Forever,            "Forever",             cc,{}},
        {BlockType::If_Then,            "If (  ) then",        cc,{}},
        {BlockType::If_Then_Else,       "If (  ) then else",   cc,{}},
        {BlockType::WaitUntil,          "Wait until (  )",     cc,{}},
        {BlockType::RepeatUntil,        "Repeat until (  )",   cc,{}},
        {BlockType::StopAll,            "Stop all",            cc,{}},
    };
    SDL_Color sec={92,177,214,255};
    tabs[Placing_Tab_SENSING]={
        {BlockType::TouchingMousePointer,"Touching mouse?",    sec,{}},
        {BlockType::TouchingEdge,        "Touching edge?",     sec,{}},
        {BlockType::TouchingSprite,      "Touching sprite?",   sec,{}},
        {BlockType::DistanceToMouse,     "Distance to mouse",  sec,{}},
        {BlockType::DistanceToSprite,    "Distance to sprite", sec,{}},
        {BlockType::MouseX,              "Mouse X",            sec,{}},
        {BlockType::MouseY,              "Mouse Y",            sec,{}},
        {BlockType::MouseDown,           "Mouse down?",        sec,{}},
        {BlockType::KeyPressed,          "Key (  ) pressed?",  sec,{}},
        {BlockType::Timer,               "Timer",              sec,{}},
        {BlockType::ResetTimer,          "Reset timer",        sec,{}},
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
    ui.Placing_ettePanel={0,0,shop_screen_width,window_screen_height};
    int tbw=shop_screen_width/2,tbh=40;
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

struct SpriteEntry {
    Sprite sprite;
    string name;
    SDL_Texture *thumbnail = nullptr;
    vector<string> soundNames;
    vector<string> costumePaths;
    vector<string> soundFilePaths;
};

struct AppState {
    int  Placing_Tab     = Placing_Tab_MOTION;
    int  placeTab        = Scripts_Tab;
    int  activeSprite    = 0;
    bool greenFlagOn     = false;
    bool running         = true;
    int  Placing_ScrollY = 0;
    int  dragSrcIdx=-1, dragOffX=0, dragOffY=0;
    int  dragScriptIdx=-1, dragScriptOffX=0, dragScriptOffY=0;
    int  editBlockIdx=-1, editInputIdx=-1;
    int  mouseX=0, mouseY=0;
    bool contextMenuOpen = false;
    int  contextSpriteIdx=-1, contextMenuX=0, contextMenuY=0;
    bool costumeCtx_Open = false;
    int  selected_costume_id=-1, costumeCtx_X=0, costumeCtx_Y=0;
};

static SDL_Texture *launchCostumeEditor(SDL_Renderer *renderer, const string &savePath) {
    {
        ifstream test(savePath);
        if (!test.good()) {
            constexpr int W=480,H=360;
            SDL_Surface *blank=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_RGBA8888);
            if (blank) {
                SDL_FillRect(blank,nullptr,SDL_MapRGBA(blank->format,255,255,255,0));
                IMG_SavePNG(blank,savePath.c_str());
                SDL_FreeSurface(blank);
            }
        }
    }
    string cmd="./costume_editor \""+savePath+"\"";
    system(cmd.c_str());
    return IMG_LoadTexture(renderer,savePath.c_str());
}

static bool promptAndLoadSound(SDL_Renderer*, SpriteEntry &) { return false; }

int main() {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0){cout<<SDL_GetError();return 1;}
    if (TTF_Init()<0){cout<<TTF_GetError();return 1;}
    if (!IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG)){cout<<IMG_GetError();return 1;}
    soundInitializer();

    SDL_Window *window=SDL_CreateWindow("Scratch IDE – SDL2/C++",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        window_screen_width,window_screen_height,SDL_WINDOW_SHOWN);
    if (!window){cout<<SDL_GetError();return 1;}

    SDL_Renderer *renderer=SDL_CreateRenderer(window,-1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){cout<<SDL_GetError();return 1;}

    FontLarge=TTF_OpenFont("./Fonts/arial.ttf",17);
    FontSmall=TTF_OpenFont("./Fonts/arial.ttf",13);
    if (!FontLarge||!FontSmall) cout<<"[warn] font not loaded\n";

    {
        SayBubbleTex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,1,1);
        Uint32 white=0xFFFFFFFF; SDL_UpdateTexture(SayBubbleTex,nullptr,&white,4);
    }
    {
        ThinkBubbleTex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,1,1);
        Uint32 lavender=0xE8D5FFFF; SDL_UpdateTexture(ThinkBubbleTex,nullptr,&lavender,4);
    }

    setGameArea(stage_starting_x,60,satge_width,window_screen_height-160);

    auto palette=buildPalette();
    UIRects ui=buildRects();
    AppState state;
    vector<ScriptBlock> scriptBlocks;

    vector<SpriteEntry> sprites;
    {
        SpriteEntry sp; sp.name="Sprite1";
        SDL_Texture *pt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,1,1);
        Uint32 white=0xFFFFFFFF; SDL_UpdateTexture(pt,nullptr,&white,4);
        sp.sprite=createSprite(pt,"default",stage_starting_x*2+satge_width,(60)*2+(window_screen_height-160));
        sp.thumbnail=pt; sp.costumePaths.push_back("");
        sprites.push_back(sp);
    }
    Stage stage; setUpStage(stage);
    SDL_StartTextInput();

    SDL_Event sdlEvent;
    while (state.running) {
        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
                case SDL_TEXTINPUT:
                    if (state.editBlockIdx>=0&&state.editInputIdx>=0) {
                        auto &sb=scriptBlocks[state.editBlockIdx];
                        if (state.editInputIdx<(int)sb.input_Values.size())
                            sb.input_Values[state.editInputIdx]+=sdlEvent.text.text;
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (sdlEvent.key.keysym.sym) {
                        case SDLK_BACKSPACE:
                            if (state.editBlockIdx>=0&&state.editInputIdx>=0) {
                                auto &val=scriptBlocks[state.editBlockIdx].input_Values[state.editInputIdx];
                                if (!val.empty()) val.pop_back();
                            }
                            break;
                        case SDLK_RETURN: case SDLK_ESCAPE:
                            if (state.editBlockIdx>=0) scriptBlocks[state.editBlockIdx].active_Input=-1;
                            state.editBlockIdx=state.editInputIdx=-1;
                            break;
                        default: break;
                    }
                    break;
                case SDL_QUIT: state.running=false; break;
                case SDL_MOUSEMOTION:
                    state.mouseX=sdlEvent.motion.x; state.mouseY=sdlEvent.motion.y;
                    if (state.dragScriptIdx>=0) {
                        auto &sb=scriptBlocks[state.dragScriptIdx];
                        sb.x=state.mouseX-state.dragScriptOffX;
                        sb.y=state.mouseY-state.dragScriptOffY;
                        sb.snapParent=-1;
                        propagateSnap(scriptBlocks,state.dragScriptIdx);
                    }
                    if (!sprites.empty()) spriteDrag(sprites[state.activeSprite].sprite,sdlEvent);
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

            if (sdlEvent.type==SDL_MOUSEBUTTONDOWN&&sdlEvent.button.button==SDL_BUTTON_LEFT) {
                SDL_Point mp={sdlEvent.button.x,sdlEvent.button.y};

                if (state.contextMenuOpen) {
                    SDL_Rect editBtn={state.contextMenuX,state.contextMenuY,180,28};
                    SDL_Rect delBtn ={state.contextMenuX,state.contextMenuY+28,180,28};
                    if (SDL_PointInRect(&mp,&editBtn)) {
                        int idx=state.contextSpriteIdx;
                        if (idx>=0&&idx<(int)sprites.size()) {
                            auto &sp=sprites[idx];
                            string savePath;
                            if (!sp.sprite.spriteCostumes.empty()) {
                                const string &cn=sp.sprite.spriteCostumes[sp.sprite.currentCostumeIndex].costumeName;
                                if (cn.size()>4&&cn.substr(cn.size()-4)==".png") savePath=cn;
                            }
                            if (savePath.empty()) savePath="./sprites/costume_"+sp.name+".png";
                            SDL_Texture *newTex=launchCostumeEditor(renderer,savePath);
                            if (newTex) {
                                int ci=sp.sprite.currentCostumeIndex;
                                sp.sprite.spriteCostumes[ci].costumeTexture=newTex;
                                sp.sprite.spriteCostumes[ci].costumeName=savePath;
                                SDL_QueryTexture(newTex,nullptr,nullptr,&sp.sprite.costumeWidth,&sp.sprite.costumeHeight);
                                sp.thumbnail=newTex;
                                if (ci<(int)sp.costumePaths.size()) sp.costumePaths[ci]=savePath;
                                else { sp.costumePaths.resize(ci+1,""); sp.costumePaths[ci]=savePath; }
                            }
                        }
                        state.contextMenuOpen=false; continue;
                    } else if (SDL_PointInRect(&mp,&delBtn)) {
                        int idx=state.contextSpriteIdx;
                        if (idx>=0&&idx<(int)sprites.size()) {
                            freeAllSounds(sprites[idx].sprite);
                            sprites.erase(sprites.begin()+idx);
                            if (state.activeSprite>=(int)sprites.size())
                                state.activeSprite=max(0,(int)sprites.size()-1);
                        }
                        state.contextMenuOpen=false; continue;
                    } else { state.contextMenuOpen=false; }
                }

                if (state.costumeCtx_Open) {
                    SDL_Rect editBtn={state.costumeCtx_X,state.costumeCtx_Y,180,28};
                    SDL_Rect delBtn ={state.costumeCtx_X,state.costumeCtx_Y+28,180,28};
                    if (SDL_PointInRect(&mp,&editBtn)) {
                        if (!sprites.empty()) {
                            auto &sp=sprites[state.activeSprite];
                            int ci=state.selected_costume_id;
                            if (ci>=0&&ci<(int)sp.sprite.spriteCostumes.size()) {
                                string savePath;
                                if (ci<(int)sp.costumePaths.size()&&!sp.costumePaths[ci].empty())
                                    savePath=sp.costumePaths[ci];
                                else savePath="./sprites/costume_"+sp.name+"_"+to_string(ci)+".png";
                                SDL_Texture *newTex=launchCostumeEditor(renderer,savePath);
                                if (newTex) {
                                    sp.sprite.spriteCostumes[ci].costumeTexture=newTex;
                                    sp.sprite.spriteCostumes[ci].costumeName=savePath;
                                    SDL_QueryTexture(newTex,nullptr,nullptr,&sp.sprite.costumeWidth,&sp.sprite.costumeHeight);
                                    if (ci==sp.sprite.currentCostumeIndex) sp.thumbnail=newTex;
                                    if (ci<(int)sp.costumePaths.size()) sp.costumePaths[ci]=savePath;
                                    else { sp.costumePaths.resize(ci+1,""); sp.costumePaths[ci]=savePath; }
                                }
                            }
                        }
                        state.costumeCtx_Open=false; continue;
                    } else if (SDL_PointInRect(&mp,&delBtn)) {
                        if (!sprites.empty()) {
                            auto &sp=sprites[state.activeSprite];
                            int ci=state.selected_costume_id;
                            int cn=(int)sp.sprite.spriteCostumes.size();
                            if (cn>1&&ci>=0&&ci<cn) {
                                sp.sprite.spriteCostumes.erase(sp.sprite.spriteCostumes.begin()+ci);
                                if (ci<(int)sp.costumePaths.size()) sp.costumePaths.erase(sp.costumePaths.begin()+ci);
                                if (sp.sprite.currentCostumeIndex>=(int)sp.sprite.spriteCostumes.size())
                                    sp.sprite.currentCostumeIndex=(int)sp.sprite.spriteCostumes.size()-1;
                                sp.thumbnail=sp.sprite.spriteCostumes[sp.sprite.currentCostumeIndex].costumeTexture;
                                SDL_QueryTexture(sp.thumbnail,nullptr,nullptr,&sp.sprite.costumeWidth,&sp.sprite.costumeHeight);
                            }
                        }
                        state.costumeCtx_Open=false; continue;
                    } else { state.costumeCtx_Open=false; }
                }

                for (int i=0;i<Placing_Tab_COUNT;i++)
                    if (SDL_PointInRect(&mp,&ui.Placing_TabButtons[i])){state.Placing_Tab=i;state.Placing_ScrollY=0;}
                if (SDL_PointInRect(&mp,&ui.scriptButton))  state.placeTab=Scripts_Tab;
                if (SDL_PointInRect(&mp,&ui.costumeButton)) state.placeTab=Customs_Tab;
                if (SDL_PointInRect(&mp,&ui.soundButton))   state.placeTab=Sounds_Tab;

                if (state.placeTab==Customs_Tab&&!sprites.empty()) {
                    SDL_Rect cosBtn={ui.scriptCanvas.x+10,ui.scriptCanvas.y+10,160,34};
                    if (SDL_PointInRect(&mp,&cosBtn)) {
                        auto &sp=sprites[state.activeSprite];
                        string savePath="./sprites/costume_"+sp.name+"_"+to_string(sp.sprite.currentCostumeIndex)+".png";
                        SDL_Texture *newTex=launchCostumeEditor(renderer,savePath);
                        if (newTex) {
                            addCostume(sp.sprite,newTex,savePath);
                            switchCostumeTo(sp.sprite,savePath);
                            sp.thumbnail=newTex;
                            sp.costumePaths.push_back(savePath);
                        }
                    }
                    auto &sp=sprites[state.activeSprite];
                    int caX=ui.scriptCanvas.x+10,caY=ui.scriptCanvas.y+60;
                    for (int i=0;i<(int)sp.sprite.spriteCostumes.size();i++) {
                        SDL_Rect frame={caX,caY+i*90,80,80};
                        if (SDL_PointInRect(&mp,&frame)) {
                            sp.sprite.currentCostumeIndex=i;
                            sp.thumbnail=sp.sprite.spriteCostumes[i].costumeTexture;
                        }
                    }
                }

                if (state.placeTab==Sounds_Tab&&!sprites.empty()) {
                    SDL_Rect sndBtn={ui.scriptCanvas.x+10,ui.scriptCanvas.y+10,140,34};
                    if (SDL_PointInRect(&mp,&sndBtn)) promptAndLoadSound(renderer,sprites[state.activeSprite]);
                }

                if (SDL_PointInRect(&mp,&ui.greenFlagButton)) state.greenFlagOn=true;
                if (SDL_PointInRect(&mp,&ui.stopButton))      state.greenFlagOn=false;

                if (SDL_PointInRect(&mp,&ui.addSpriteButton)) {
                    string savePath="./sprites/costume_"+to_string(sprites.size()+1)+".png";
                    SDL_Texture *costumeTex=launchCostumeEditor(renderer,savePath);
                    SpriteEntry sp; sp.name="Sprite"+to_string(sprites.size()+1);
                    if (costumeTex) {
                        sp.sprite=createSprite(costumeTex,savePath,stage_starting_x*2+satge_width,(60)*2+(window_screen_height-160));
                        sp.thumbnail=costumeTex; sp.costumePaths.push_back(savePath);
                    } else {
                        SDL_Texture *pt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,1,1);
                        Uint32 c=0xFF6464FF; SDL_UpdateTexture(pt,nullptr,&c,4);
                        sp.sprite=createSprite(pt,"default",stage_starting_x*2+satge_width,(60)*2+(window_screen_height-160));
                        sp.thumbnail=pt; sp.costumePaths.push_back("");
                    }
                    sprites.push_back(sp);
                    state.activeSprite=(int)sprites.size()-1;
                }

                {
                    int tw=70,th=70,tp=8;
                    int startX=ui.addSpriteButton.x+ui.addSpriteButton.w+tp;
                    for (int i=0;i<(int)sprites.size();i++) {
                        SDL_Rect tr={startX+i*(tw+tp),ui.spriteListPanel.y+8,tw,th};
                        if (SDL_PointInRect(&mp,&tr)) state.activeSprite=i;
                    }
                }

                if (SDL_PointInRect(&mp,&ui.scriptCanvas)&&state.placeTab==Scripts_Tab) {
                    bool consumed=false;
                    for (int i=(int)scriptBlocks.size()-1;i>=0&&!consumed;i--) {
                        auto &sb=scriptBlocks[i];
                        for (int j=0;j<(int)sb.input_Rects.size()&&!consumed;j++) {
                            if (SDL_PointInRect(&mp,&sb.input_Rects[j])) {
                                if (state.editBlockIdx>=0&&state.editBlockIdx!=i)
                                    scriptBlocks[state.editBlockIdx].active_Input=-1;
                                sb.active_Input=j;
                                state.editBlockIdx=i; state.editInputIdx=j;
                                consumed=true;
                            }
                        }
                    }
                    if (!consumed) {
                        if (state.editBlockIdx>=0) scriptBlocks[state.editBlockIdx].active_Input=-1;
                        state.editBlockIdx=state.editInputIdx=-1;
                        for (int i=(int)scriptBlocks.size()-1;i>=0&&!consumed;i--) {
                            SDL_Rect br={scriptBlocks[i].x,scriptBlocks[i].y,220,block_height};
                            if (SDL_PointInRect(&mp,&br)) {
                                state.dragScriptIdx=i;
                                state.dragScriptOffX=mp.x-scriptBlocks[i].x;
                                state.dragScriptOffY=mp.y-scriptBlocks[i].y;
                                scriptBlocks[i].snapParent=-1;
                                consumed=true;
                            }
                        }
                    }
                }

                if (SDL_PointInRect(&mp,&ui.Placing_BlockArea)&&state.placeTab==Scripts_Tab) {
                    auto &cat=palette[state.Placing_Tab];
                    for (int i=0;i<(int)cat.size();i++) {
                        SDL_Rect pr={ui.Placing_BlockArea.x+8,
                                     ui.Placing_BlockArea.y+8+i*(block_height+blocks_gap)-state.Placing_ScrollY,
                                     ui.Placing_BlockArea.w-16,block_height};
                        if (SDL_PointInRect(&mp,&pr)) {
                            state.dragSrcIdx=i;
                            state.dragOffX=mp.x-pr.x;
                            state.dragOffY=mp.y-pr.y;
                        }
                    }
                }

                if (!sprites.empty()) spriteDrag(sprites[state.activeSprite].sprite,sdlEvent);
            }

            if (sdlEvent.type==SDL_MOUSEBUTTONDOWN&&sdlEvent.button.button==SDL_BUTTON_RIGHT) {
                SDL_Point mp={sdlEvent.button.x,sdlEvent.button.y};
                state.contextMenuOpen=false; state.costumeCtx_Open=false;
                int tw=70,th=70,tp=8;
                int startX=ui.addSpriteButton.x+ui.addSpriteButton.w+tp;
                for (int i=0;i<(int)sprites.size();i++) {
                    SDL_Rect tr={startX+i*(tw+tp),ui.spriteListPanel.y+8,tw,th};
                    if (SDL_PointInRect(&mp,&tr)) {
                        state.contextSpriteIdx=i;
                        state.contextMenuX=sdlEvent.button.x;
                        state.contextMenuY=sdlEvent.button.y;
                        state.contextMenuOpen=true;
                    }
                }
                if (state.placeTab==Customs_Tab&&!sprites.empty()) {
                    auto &sp=sprites[state.activeSprite];
                    int caX=ui.scriptCanvas.x+10,caY=ui.scriptCanvas.y+60;
                    for (int i=0;i<(int)sp.sprite.spriteCostumes.size();i++) {
                        SDL_Rect frame={caX,caY+i*90,80,80};
                        if (SDL_PointInRect(&mp,&frame)) {
                            state.selected_costume_id=i;
                            state.costumeCtx_X=sdlEvent.button.x;
                            state.costumeCtx_Y=sdlEvent.button.y;
                            state.costumeCtx_Open=true;
                        }
                    }
                }
            }

            if (sdlEvent.type==SDL_MOUSEBUTTONUP&&sdlEvent.button.button==SDL_BUTTON_LEFT) {
                SDL_Point mp={sdlEvent.button.x,sdlEvent.button.y};
                if (state.dragSrcIdx>=0) {
                    if (SDL_PointInRect(&mp,&ui.scriptCanvas)&&state.placeTab==Scripts_Tab) {
                        ScriptBlock db;
                        db.placed_block_in_canvas=palette[state.Placing_Tab][state.dragSrcIdx];
                        db.x=mp.x-state.dragOffX; db.y=mp.y-state.dragOffY;
                        db.input_Values.resize(countInputs(db.placed_block_in_canvas.label));
                        auto sr=findSnapTarget(scriptBlocks,-1,db.x,db.y);
                        if (sr.targetIdx>=0) {
                            auto &tgt=scriptBlocks[sr.targetIdx];
                            db.x=tgt.x; db.y=tgt.y+blockTotalHeight(scriptBlocks,sr.targetIdx);
                            db.snapParent=sr.targetIdx;
                            db.container_Parent=tgt.container_Parent;
                            db.container_Branch=tgt.container_Branch;
                        } else if (sr.intoContainer) {
                            db.x=scriptBlocks[sr.containerIdx].x+CONTAINER_INDENT;
                            db.y=containerInnerY(scriptBlocks,sr.containerIdx,sr.containerBranch);
                            db.container_Parent=sr.containerIdx;
                            db.container_Branch=sr.containerBranch;
                        }
                        scriptBlocks.push_back(move(db));
                        if (scriptBlocks.back().container_Parent>=0)
                            propagateContainerChildren(scriptBlocks,scriptBlocks.back().container_Parent);
                    }
                    state.dragSrcIdx=-1;
                }
                if (state.dragScriptIdx>=0) {
                    auto &db=scriptBlocks[state.dragScriptIdx];
                    if (!SDL_PointInRect(&mp,&ui.scriptCanvas)) {
                        scriptBlocks.erase(scriptBlocks.begin()+state.dragScriptIdx);
                        for (auto &b:scriptBlocks) {
                            if (b.snapParent==state.dragScriptIdx) b.snapParent=-1;
                            else if (b.snapParent>state.dragScriptIdx) b.snapParent--;
                            if (b.container_Parent==state.dragScriptIdx) b.container_Parent=-1;
                            else if (b.container_Parent>state.dragScriptIdx) b.container_Parent--;
                        }
                    } else {
                        db.snapParent=-1;
                        int oldCon=db.container_Parent;
                        db.container_Parent=-1;
                        auto sr=findSnapTarget(scriptBlocks,state.dragScriptIdx,db.x,db.y);
                        if (sr.targetIdx>=0) {
                            auto &tgt=scriptBlocks[sr.targetIdx];
                            db.x=tgt.x; db.y=tgt.y+blockTotalHeight(scriptBlocks,sr.targetIdx);
                            db.snapParent=sr.targetIdx;
                            db.container_Parent=tgt.container_Parent;
                            db.container_Branch=tgt.container_Branch;
                        } else if (sr.intoContainer) {
                            db.x=scriptBlocks[sr.containerIdx].x+CONTAINER_INDENT;
                            db.y=containerInnerY(scriptBlocks,sr.containerIdx,sr.containerBranch);
                            db.container_Parent=sr.containerIdx;
                            db.container_Branch=sr.containerBranch;
                        }
                        propagateSnap(scriptBlocks,state.dragScriptIdx);
                        if (oldCon>=0) propagateContainerChildren(scriptBlocks,oldCon);
                        if (db.container_Parent>=0) propagateContainerChildren(scriptBlocks,db.container_Parent);
                    }
                    state.dragScriptIdx=-1;
                }
                if (!sprites.empty()) spriteDrag(sprites[state.activeSprite].sprite,sdlEvent);
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
                SDL_Rect sep={ui.Placing_TabButtons[i].x,ui.Placing_TabButtons[i].y+ui.Placing_TabButtons[i].h-1,ui.Placing_TabButtons[i].w,1};
                SDL_RenderFillRect(renderer,&sep);
            }
            SDL_RenderSetClipRect(renderer,&ui.Placing_BlockArea);
            auto &cat=palette[state.Placing_Tab];
            SDL_Color pc=placing_tab_colors[state.Placing_Tab];
            for (int i=0;i<(int)cat.size();i++) {
                SDL_Rect r={ui.Placing_BlockArea.x+8,ui.Placing_BlockArea.y+8+i*(block_height+blocks_gap)-state.Placing_ScrollY,ui.Placing_BlockArea.w-16,block_height};
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

            if (state.placeTab==Scripts_Tab) {
                SDL_RenderSetClipRect(renderer,&ui.scriptCanvas);
                auto drawSnapGuide=[&](int movingIdx,int mx,int my){
                    auto sr=findSnapTarget(scriptBlocks,movingIdx,mx,my);
                    SDL_SetRenderDrawColor(renderer,255,240,80,220);
                    if (sr.targetIdx>=0) {
                        int lineY=scriptBlocks[sr.targetIdx].y+blockTotalHeight(scriptBlocks,sr.targetIdx);
                        SDL_RenderDrawLine(renderer,scriptBlocks[sr.targetIdx].x,lineY,scriptBlocks[sr.targetIdx].x+220,lineY);
                    } else if (sr.intoContainer) {
                        auto &c=scriptBlocks[sr.containerIdx];
                        int lineY=containerInnerY(scriptBlocks,sr.containerIdx,sr.containerBranch);
                        SDL_RenderDrawLine(renderer,c.x+CONTAINER_INDENT,lineY,c.x+CONTAINER_INDENT+200,lineY);
                    }
                };
                if (state.dragScriptIdx>=0)
                    drawSnapGuide(state.dragScriptIdx,scriptBlocks[state.dragScriptIdx].x,scriptBlocks[state.dragScriptIdx].y);
                if (state.dragSrcIdx>=0)
                    drawSnapGuide(-1,state.mouseX-state.dragOffX,state.mouseY-state.dragOffY);
                for (int i=0;i<(int)scriptBlocks.size();i++)
                    renderScriptBlock(renderer,scriptBlocks[i],scriptBlocks,i);
                SDL_RenderSetClipRect(renderer,nullptr);
                if (scriptBlocks.empty())
                    renderTextCentered(renderer,FontSmall,"Drag blocks here from the palette",ui.scriptCanvas,{85,85,100,255});
                renderText(renderer,FontSmall,"Click a white box to edit its value  |  Drag block outside to delete",
                    ui.scriptCanvas.x+8,ui.scriptCanvas.y+ui.scriptCanvas.h-18,{70,70,85,255});

            } else if (state.placeTab==Customs_Tab) {
                SDL_Rect cosBtn={ui.scriptCanvas.x+10,ui.scriptCanvas.y+10,160,34};
                drawRect(renderer,cosBtn,{70,130,200,255});
                renderTextCentered(renderer,FontSmall,"+ Change Costume",cosBtn);
                if (!sprites.empty()) {
                    auto &sp=sprites[state.activeSprite];
                    int csX=ui.scriptCanvas.x+10,csY=ui.scriptCanvas.y+60;
                    for (int i=0;i<(int)sp.sprite.spriteCostumes.size();i++) {
                        SDL_Rect frame={csX,csY+i*90,80,80};
                        SDL_SetRenderDrawColor(renderer,
                            i==sp.sprite.currentCostumeIndex?100:60,
                            i==sp.sprite.currentCostumeIndex?160:60,
                            i==sp.sprite.currentCostumeIndex?240:70,255);
                        SDL_RenderFillRect(renderer,&frame);
                        if (sp.sprite.spriteCostumes[i].costumeTexture)
                            SDL_RenderCopy(renderer,sp.sprite.spriteCostumes[i].costumeTexture,nullptr,&frame);
                        renderText(renderer,FontSmall,"Costume "+to_string(i+1),csX+86,csY+i*90+32,{200,200,200,255});
                        if (i==sp.sprite.currentCostumeIndex)
                            renderText(renderer,FontSmall,"(active)",csX+86,csY+i*90+50,{120,200,120,255});
                    }
                } else {
                    renderTextCentered(renderer,FontSmall,"Add a sprite first",ui.scriptCanvas,{85,85,100,255});
                }
            } else {
                SDL_Rect sndBtn={ui.scriptCanvas.x+10,ui.scriptCanvas.y+10,140,34};
                drawRect(renderer,sndBtn,{220,105,132,255});
                renderTextCentered(renderer,FontSmall,"+ Add Sound",sndBtn);
                if (!sprites.empty()) {
                    auto &sp=sprites[state.activeSprite];
                    if (sp.soundNames.empty()) {
                        renderText(renderer,FontSmall,"No sounds yet. Click \"+ Add Sound\" to load a WAV / MP3.",
                            ui.scriptCanvas.x+10,ui.scriptCanvas.y+60,{120,120,140,255});
                    } else {
                        renderText(renderer,FontSmall,"Sounds for "+sp.name+":",
                            ui.scriptCanvas.x+10,ui.scriptCanvas.y+56,{200,200,200,255});
                        for (int i=0;i<(int)sp.soundNames.size();i++) {
                            SDL_Rect row={ui.scriptCanvas.x+10,ui.scriptCanvas.y+76+i*38,ui.scriptCanvas.w-20,32};
                            SDL_SetRenderDrawColor(renderer,65,50,75,255);
                            SDL_RenderFillRect(renderer,&row);
                            renderText(renderer,FontSmall,"♪  "+sp.soundNames[i],row.x+8,row.y+8,{230,180,220,255});
                        }
                    }
                } else {
                    renderTextCentered(renderer,FontSmall,"Add a sprite first",ui.scriptCanvas,{85,85,100,255});
                }
            }
        }

        {
            SDL_SetRenderDrawColor(renderer,35,35,40,255);
            SDL_RenderFillRect(renderer,&ui.stagePanel);
            drawRect(renderer,ui.greenFlagButton,state.greenFlagOn?SDL_Color{50,200,80,255}:SDL_Color{30,140,50,255});
            renderTextCentered(renderer,FontLarge,">",ui.greenFlagButton);
            drawRect(renderer,ui.stopButton,{180,50,50,255});
            renderTextCentered(renderer,FontLarge,"■",ui.stopButton);
            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderFillRect(renderer,&ui.stageView);
            drawStage(renderer,stage);
            for (auto &sp:sprites) drawSprite(renderer,sp.sprite,SayBubbleTex,ThinkBubbleTex);
            SDL_SetRenderDrawColor(renderer,80,80,90,255);
            SDL_RenderDrawRect(renderer,&ui.stageView);
            SDL_SetRenderDrawColor(renderer,45,45,55,255);
            SDL_RenderFillRect(renderer,&ui.spriteListPanel);
            drawRect(renderer,ui.addSpriteButton,{70,130,200,255});
            renderTextCentered(renderer,FontSmall,"+ Sprite",ui.addSpriteButton);
            int tw=70,th=70,tp=8;
            int startX=ui.addSpriteButton.x+ui.addSpriteButton.w+tp;
            for (int i=0;i<(int)sprites.size();i++) {
                SDL_Rect tr={startX+i*(tw+tp),ui.spriteListPanel.y+8,tw,th};
                if (i==state.activeSprite) {
                    SDL_SetRenderDrawColor(renderer,100,160,240,255);
                    SDL_Rect hl={tr.x-2,tr.y-2,tr.w+4,tr.h+4};
                    SDL_RenderFillRect(renderer,&hl);
                }
                SDL_SetRenderDrawColor(renderer,60,60,70,255);
                SDL_RenderFillRect(renderer,&tr);
                if (sprites[i].thumbnail) SDL_RenderCopy(renderer,sprites[i].thumbnail,nullptr,&tr);
                renderText(renderer,FontSmall,sprites[i].name,tr.x+2,tr.y+tr.h+2,{200,200,200,255});
            }
        }

        if (state.dragSrcIdx>=0) {
            auto &cat=palette[state.Placing_Tab];
            if (state.dragSrcIdx<(int)cat.size()) {
                vector<ScriptBlock> ghostVec(1);
                ghostVec[0].placed_block_in_canvas=cat[state.dragSrcIdx];
                ghostVec[0].x=state.mouseX-state.dragOffX;
                ghostVec[0].y=state.mouseY-state.dragOffY;
                ghostVec[0].input_Values.resize(countInputs(ghostVec[0].placed_block_in_canvas.label));
                renderScriptBlock(renderer,ghostVec[0],ghostVec,0,true);
            }
        }

        SDL_SetRenderDrawColor(renderer,15,15,15,255);
        SDL_Rect ld={shop_screen_width-2,0,2,window_screen_height};
        SDL_Rect md={shop_screen_width+placing_screen_width-2,0,2,window_screen_height};
        SDL_RenderFillRect(renderer,&ld);
        SDL_RenderFillRect(renderer,&md);

        auto drawContextMenu=[&](int x,int y,const char *items[],SDL_Color itemBg[],int count){
            int menuW=180,itemH=28;
            for (int i=0;i<count;i++) {
                SDL_Rect row={x,y+i*itemH,menuW,itemH};
                SDL_Rect shadow={row.x+2,row.y+2,row.w,row.h};
                SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer,0,0,0,120);
                SDL_RenderFillRect(renderer,&shadow);
                SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
                drawRect(renderer,row,itemBg[i]);
                SDL_SetRenderDrawColor(renderer,200,200,200,255);
                SDL_RenderDrawRect(renderer,&row);
                renderTextCentered(renderer,FontSmall,items[i],row);
            }
        };

        if (state.contextMenuOpen&&state.contextSpriteIdx>=0&&state.contextSpriteIdx<(int)sprites.size()) {
            const char *items[]={"Edit Costume","Delete Sprite"};
            SDL_Color cols[]={{60,100,180,255},{180,60,60,255}};
            drawContextMenu(state.contextMenuX,state.contextMenuY,items,cols,2);
        }
        if (state.costumeCtx_Open&&!sprites.empty()&&state.selected_costume_id>=0
            &&state.selected_costume_id<(int)sprites[state.activeSprite].sprite.spriteCostumes.size()) {
            bool canDelete=(int)sprites[state.activeSprite].sprite.spriteCostumes.size()>1;
            const char *items[]={"Edit Costume",canDelete?"Delete Costume":"(can't delete last)"};
            SDL_Color cols[]={{60,100,180,255},canDelete?SDL_Color{180,60,60,255}:SDL_Color{80,80,80,255}};
            drawContextMenu(state.costumeCtx_X,state.costumeCtx_Y,items,cols,2);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    for (auto &sp:sprites) freeAllSounds(sp.sprite);
    if (FontSmall) TTF_CloseFont(FontSmall);
    if (FontLarge) TTF_CloseFont(FontLarge);
    if (SayBubbleTex)   SDL_DestroyTexture(SayBubbleTex);
    if (ThinkBubbleTex) SDL_DestroyTexture(ThinkBubbleTex);
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit(); TTF_Quit(); SDL_Quit();
    return 0;
}