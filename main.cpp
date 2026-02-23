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
const int window_screen_height  = 900;
const int shop_screen_width    = 300;
const int placing_screen_width   = 500;
const int satge_width   = window_screen_width - shop_screen_width - placing_screen_width;
const int stage_starting_x   = shop_screen_width + placing_screen_width;
const int block_height   = 34;
const int blocks_gap = 6;
const int snap_area = 22;
const int input_width   = 46;
const int input_height   = 22;

const int Scripts_Tab=0, Customs_Tab=1, Sounds_Tab=2;
const int Placing_Tab_MOTION=0, Placing_Tab_LOOKS=1, Placing_Tab_SOUND=2, Placing_Tab_CONTROL=3,Placing_Tab_SENSING=4, Placing_Tab_OPERATORS=5, Placing_Tab_VARIABLES=6, Placing_Tab_COUNT=7;

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
    int inputCount = 0;
    size_t text_size = 0;
    string input_buffer;
    while (text_size < label.size()) {
        if (label[text_size] == '(') {
            if (!input_buffer.empty()) { parts.push_back({false, input_buffer, -1}); input_buffer.clear(); }
            size_t j = label.find(')', text_size);
            if (j == string::npos) j = label.size()-1;
            parts.push_back({true, label.substr(text_size+1, j-text_size-1), inputCount++});
            text_size = j+1;
        } else {
            input_buffer += label[text_size++];
        }
    }
    if (!input_buffer.empty()) parts.push_back({false, input_buffer, -1});
    return parts;
}

static int countInputs(const string &label) {
    int inputCount=0; for (char character:label) if (character=='(') inputCount++; return inputCount;
}

const int CONTAINER_INDENT = 24;
const int CONTAINER_ARM_W  = 14;
const int CONTAINER_BOT_H  = block_height;
const int CONTAINER_MIN_INNER_H = block_height;

static bool isContainerBlock(BlockType investigating_block) {
    return (investigating_block == BlockType::Repeat || investigating_block == BlockType::Forever ||  investigating_block == BlockType::If_Then || investigating_block == BlockType::If_Then_Else|| investigating_block == BlockType::RepeatUntil || investigating_block == BlockType::WaitUntil);
}
static bool hasElseBranch(BlockType investigating_block) {
    return investigating_block == BlockType::If_Then_Else;
}

struct ScriptBlock {
    Blocks_In_IDE placed_block_in_canvas;
    int x, y;
    vector<string> input_Values;
    vector<SDL_Rect> input_Rects;
    int active_Input = -1;
    int snapParent = -1;
    int container_Parent = -1;
    int container_Branch = 0;
};

static int blockTotalHeight(const vector<ScriptBlock> &blocks, int ID_X);

static int blockTotalHeight(const vector<ScriptBlock> &blocks, int ID_X) {
    const auto &scriptBlockRef = blocks[ID_X];
    if (!isContainerBlock(scriptBlockRef.placed_block_in_canvas.block_type))
        return block_height;

    int Heights_of_in_branch1 = 0;
    for (int i = 0; i < (int)blocks.size(); i++) {
        if (blocks[i].container_Parent == ID_X && blocks[i].container_Branch == 0 && blocks[i].snapParent == -1) {

            int current_chain_block_index = i;
            while (current_chain_block_index != -1) {
                Heights_of_in_branch1 += blockTotalHeight(blocks, current_chain_block_index);

                int next_snap_block_index = -1;
                for (int k = 0; k < (int)blocks.size(); k++)
                    if (blocks[k].snapParent == current_chain_block_index && blocks[k].container_Parent == ID_X
                        && blocks[k].container_Branch == 0) { next_snap_block_index = k; break; }
                current_chain_block_index = next_snap_block_index;
            }
        }
    }

    if (Heights_of_in_branch1 < CONTAINER_MIN_INNER_H) Heights_of_in_branch1 = CONTAINER_MIN_INNER_H;

    if (!hasElseBranch(scriptBlockRef.placed_block_in_canvas.block_type))
        return block_height + Heights_of_in_branch1 + CONTAINER_BOT_H;

    int Height_of_inner_branch2 = 0;
    for (int i = 0; i < (int)blocks.size(); i++) {
        if (blocks[i].container_Parent == ID_X && blocks[i].container_Branch == 1
            && blocks[i].snapParent == -1) {

            int current_chain_block_index = i;
            while (current_chain_block_index != -1) {
                Height_of_inner_branch2 += blockTotalHeight(blocks, current_chain_block_index);
                int next_snap_block_index = -1;
                for (int k = 0; k < (int)blocks.size(); k++)
                    if (blocks[k].snapParent == current_chain_block_index && blocks[k].container_Parent == ID_X
                        && blocks[k].container_Branch == 1) { next_snap_block_index = k; break; }
                current_chain_block_index = next_snap_block_index;
            }
        }
    }
    if (Height_of_inner_branch2 < CONTAINER_MIN_INNER_H) Height_of_inner_branch2 = CONTAINER_MIN_INNER_H;

    return block_height + Heights_of_in_branch1 + CONTAINER_BOT_H + Height_of_inner_branch2 + CONTAINER_BOT_H;
}

static TTF_Font *FontLarge = nullptr;
static TTF_Font *FontSmall = nullptr;

static SDL_Texture *SayBubbleTex   = nullptr;
static SDL_Texture *ThinkBubbleTex = nullptr;

static int textWidth(TTF_Font *font, const string &text_to_render) {
    if (!font||text_to_render.empty()) return 0;
    int w=0; TTF_SizeUTF8(font,text_to_render.c_str(),&w,nullptr); return w;
}

static void renderText(SDL_Renderer *renderer, TTF_Font *font, const string &text,
                       int x, int y, SDL_Color color={255,255,255,255}) {
    if (!font||text.empty()) return;
    SDL_Surface *surf=TTF_RenderUTF8_Blended(font,text.c_str(),color);
    if (!surf) return;
    SDL_Texture *tex=SDL_CreateTextureFromSurface(renderer,surf);
    SDL_Rect destination={x,y,surf->w,surf->h};
    SDL_RenderCopy(renderer,tex,nullptr,&destination);
    SDL_DestroyTexture(tex); SDL_FreeSurface(surf);
}

static void renderTextCentered(SDL_Renderer *renderer, TTF_Font *font,
                                const string &text, SDL_Rect area,
                                SDL_Color color={255,255,255,255}) {
    if (!font||text.empty()) return;
    SDL_Surface *surf=TTF_RenderUTF8_Blended(font,text.c_str(),color);
    if (!surf) return;
    SDL_Texture *tex=SDL_CreateTextureFromSurface(renderer,surf);
    SDL_Rect destination={area.x+(area.w-surf->w)/2, area.y+(area.h-surf->h)/2,surf->w, surf->h};
    SDL_RenderCopy(renderer,tex,nullptr,&destination);
    SDL_DestroyTexture(tex); SDL_FreeSurface(surf);
}

static void drawRect(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    SDL_RenderFillRect(renderer,&rect);
}

static int renderScriptBlock(SDL_Renderer *renderer, ScriptBlock &currentBlock,
                             const vector<ScriptBlock> &allBlocks, int myIdx,
                             bool ghost=false) {
    auto parts = parseLabel(currentBlock.placed_block_in_canvas.label);

    int totalW = 12;
    for (auto &p : parts)
        totalW += p.Is_text_an_Input ? input_width+4 : textWidth(FontSmall,p.text)+4;
    if (totalW < 130) totalW = 130;

    SDL_Color blockColor = currentBlock.placed_block_in_canvas.color;
    if (ghost) { blockColor.a = 190; SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); }

    bool isContainer = isContainerBlock(currentBlock.placed_block_in_canvas.block_type);

    if (!isContainer) {

        SDL_Rect bg = {currentBlock.x, currentBlock.y, totalW, block_height};
        SDL_SetRenderDrawColor(renderer, blockColor.r, blockColor.g, blockColor.b, blockColor.a);
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer,0,0,0,70);
        SDL_Rect notch ={currentBlock.x+14, currentBlock.y+block_height-3, 16, 3};
        SDL_Rect tnotch={currentBlock.x+14, currentBlock.y,            16, 3};
        SDL_RenderFillRect(renderer,&notch);
        SDL_RenderFillRect(renderer,&tnotch);
    } else {

        int th = blockTotalHeight(allBlocks, myIdx);
        bool hasElse = hasElseBranch(currentBlock.placed_block_in_canvas.block_type);

        int innerH0 = 0;
        for (int j = 0; j < (int)allBlocks.size(); j++) {
            if (allBlocks[j].container_Parent == myIdx && allBlocks[j].container_Branch == 0
                && allBlocks[j].snapParent == -1) {
                for (int k = j; k != -1; ) {
                    innerH0 += blockTotalHeight(allBlocks, k);
                    int next = -1;
                    for (int m = 0; m < (int)allBlocks.size(); m++)
                        if (allBlocks[m].snapParent == k && allBlocks[m].container_Parent == myIdx
                            && allBlocks[m].container_Branch == 0) { next = m; break; }
                    k = next;
                }
            }
        }
        if (innerH0 < CONTAINER_MIN_INNER_H) innerH0 = CONTAINER_MIN_INNER_H;

        SDL_SetRenderDrawColor(renderer, blockColor.r, blockColor.g, blockColor.b, blockColor.a);
        SDL_Rect header = {currentBlock.x, currentBlock.y, totalW, block_height};
        SDL_RenderFillRect(renderer, &header);

        SDL_Rect arm = {currentBlock.x, currentBlock.y, CONTAINER_ARM_W, th};
        SDL_RenderFillRect(renderer, &arm);

        int capY0 = currentBlock.y + block_height + innerH0;
        SDL_Rect cap0 = {currentBlock.x, capY0, totalW, CONTAINER_BOT_H};
        SDL_RenderFillRect(renderer, &cap0);

        if (hasElse) {

            renderText(renderer, FontSmall, "else", currentBlock.x + CONTAINER_ARM_W + 6,
                       capY0 + (CONTAINER_BOT_H-13)/2);

            int innerH1 = th - block_height - innerH0 - CONTAINER_BOT_H*2;
            if (innerH1 < CONTAINER_MIN_INNER_H) innerH1 = CONTAINER_MIN_INNER_H;
            int capY1 = capY0 + CONTAINER_BOT_H + innerH1;
            SDL_Rect cap1 = {currentBlock.x, capY1, totalW, CONTAINER_BOT_H};
            SDL_RenderFillRect(renderer, &cap1);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_SetRenderDrawColor(renderer,0,0,0,70);
        SDL_Rect tnotch={currentBlock.x+14, currentBlock.y, 16, 3};
        SDL_RenderFillRect(renderer,&tnotch);

        SDL_Rect inotch={currentBlock.x+CONTAINER_ARM_W+8, currentBlock.y+block_height-3, 16, 3};
        SDL_RenderFillRect(renderer,&inotch);

        SDL_Rect bnotch={currentBlock.x+14, currentBlock.y+th-3, 16, 3};
        SDL_RenderFillRect(renderer,&bnotch);
    }

    currentBlock.input_Rects.resize(currentBlock.input_Values.size());
    int curX = isContainer ? currentBlock.x + CONTAINER_ARM_W + 6 : currentBlock.x+6;
    int midY = currentBlock.y + (block_height-13)/2;

    for (auto &p : parts) {
        if (!p.Is_text_an_Input) {
            renderText(renderer, FontSmall, p.text, curX, midY);
            curX += textWidth(FontSmall, p.text)+4;
        } else {
            bool active = (currentBlock.active_Input == p.InputID_X);
            SDL_Rect ibox = {curX, currentBlock.y+(block_height-input_height)/2, input_width, input_height};
            if (p.InputID_X < (int)currentBlock.input_Rects.size())
                currentBlock.input_Rects[p.InputID_X] = ibox;

            SDL_Color ibg = active ? SDL_Color{255,255,180,255}
                                   : SDL_Color{255,255,255,240};
            drawRect(renderer, ibox, ibg);
            SDL_SetRenderDrawColor(renderer,80,80,80,255);
            SDL_RenderDrawRect(renderer,&ibox);

            const string &val = (p.InputID_X<(int)currentBlock.input_Values.size())
                                ? currentBlock.input_Values[p.InputID_X] : "";
            if (val.empty()) {
                renderText(renderer,FontSmall,p.text, ibox.x+3, ibox.y+(input_height-12)/2,
                           {160,160,160,255});
            } else {
                renderText(renderer,FontSmall,val, ibox.x+3, ibox.y+(input_height-12)/2,
                           {0,0,0,255});
            }
            if (active && (SDL_GetTicks()/500)%2==0) {
                int cursorXPos = ibox.x+3+textWidth(FontSmall,val);
                if (cursorXPos > ibox.x+ibox.w-3) cursorXPos = ibox.x+ibox.w-3;
                SDL_SetRenderDrawColor(renderer,0,0,0,255);
                SDL_RenderDrawLine(renderer,cursorXPos,ibox.y+3,cursorXPos,ibox.y+input_height-3);
            }
            curX += input_width+4;
        }
    }

    return isContainer ? blockTotalHeight(allBlocks, myIdx) : block_height;
}

static int containerInnerY(const vector<ScriptBlock> &blocks, int cIdx, int branch=0) {
    const auto &containerBlockRef = blocks[cIdx];
    if (branch == 0) return containerBlockRef.y + block_height;

    int innerH0 = 0;
    for (int j = 0; j < (int)blocks.size(); j++) {
        if (blocks[j].container_Parent == cIdx && blocks[j].container_Branch == 0
            && blocks[j].snapParent == -1) {
            for (int k = j; k != -1; ) {
                innerH0 += blockTotalHeight(blocks, k);
                int next = -1;
                for (int m = 0; m < (int)blocks.size(); m++)
                    if (blocks[m].snapParent == k && blocks[m].container_Parent == cIdx
                        && blocks[m].container_Branch == 0) { next = m; break; }
                k = next;
            }
        }
    }
    if (innerH0 < CONTAINER_MIN_INNER_H) innerH0 = CONTAINER_MIN_INNER_H;
    return containerBlockRef.y + block_height + innerH0 + CONTAINER_BOT_H;
}

static int lastInBranch(const vector<ScriptBlock> &blocks, int cIdx, int branch) {
    int last = -1;

    for (int j = 0; j < (int)blocks.size(); j++) {
        if (blocks[j].container_Parent == cIdx && blocks[j].container_Branch == branch
            && blocks[j].snapParent == -1) {

            for (int k = j; k != -1; ) {
                last = k;
                int next = -1;
                for (int m = 0; m < (int)blocks.size(); m++)
                    if (blocks[m].snapParent == k && blocks[m].container_Parent == cIdx
                        && blocks[m].container_Branch == branch) { next = m; break; }
                k = next;
            }
        }
    }
    return last;
}

struct SnapResult {
    int  targetIdx  = -1;
    bool intoContainer = false;
    int  containerIdx  = -1;
    int  containerBranch = 0;
};

static SnapResult findSnapTarget(const vector<ScriptBlock> &blocks, int movingIdx,
                                 int mx, int my) {
    SnapResult best;

    for (int i = 0; i < (int)blocks.size(); i++) {
        if (i == movingIdx) continue;
        if (blocks[i].container_Parent == movingIdx) continue;

        bool isContainer = isContainerBlock(blocks[i].placed_block_in_canvas.block_type);

        if (isContainer) {
            int totalH = blockTotalHeight(blocks, i);

            int zone0Y = containerInnerY(blocks, i, 0);
            int zone0H = blockTotalHeight(blocks, i);

            if (mx >= blocks[i].x + CONTAINER_ARM_W - 10 && mx <= blocks[i].x + 200) {

                int b0Top = blocks[i].y + block_height;
                int innerH0 = 0;
                for (int j = 0; j < (int)blocks.size(); j++) {
                    if (blocks[j].container_Parent == i && blocks[j].container_Branch == 0
                        && blocks[j].snapParent == -1) {
                        for (int k = j; k != -1; ) {
                            innerH0 += blockTotalHeight(blocks, k);
                            int next = -1;
                            for (int m = 0; m < (int)blocks.size(); m++)
                                if (blocks[m].snapParent == k && blocks[m].container_Parent == i
                                    && blocks[m].container_Branch == 0) { next = m; break; }
                            k = next;
                        }
                    }
                }
                if (innerH0 < CONTAINER_MIN_INNER_H) innerH0 = CONTAINER_MIN_INNER_H;
                int b0Bot = b0Top + innerH0;

                if (my >= b0Top - snap_area && my <= b0Bot + snap_area) {

                    int last0 = lastInBranch(blocks, i, 0);
                    if (last0 >= 0) {

                        int botY = blocks[last0].y + blockTotalHeight(blocks, last0);
                        if (abs(my - botY) <= snap_area) {
                            best.targetIdx = last0;
                            best.intoContainer = false;
                            return best;
                        }
                    } else {

                        if (abs(my - b0Top) <= snap_area*2) {
                            best.intoContainer = true;
                            best.containerIdx = i;
                            best.containerBranch = 0;
                            return best;
                        }
                    }
                }

                if (hasElseBranch(blocks[i].placed_block_in_canvas.block_type)) {
                    int b1Top = containerInnerY(blocks, i, 1);
                    int innerH1 = totalH - block_height - innerH0 - CONTAINER_BOT_H*2;
                    if (innerH1 < CONTAINER_MIN_INNER_H) innerH1 = CONTAINER_MIN_INNER_H;
                    int b1Bot = b1Top + innerH1;

                    if (my >= b1Top - snap_area && my <= b1Bot + snap_area) {
                        int last1 = lastInBranch(blocks, i, 1);
                        if (last1 >= 0) {
                            int botY = blocks[last1].y + blockTotalHeight(blocks, last1);
                            if (abs(my - botY) <= snap_area) {
                                best.targetIdx = last1;
                                best.intoContainer = false;
                                return best;
                            }
                        } else {
                            if (abs(my - b1Top) <= snap_area*2) {
                                best.intoContainer = true;
                                best.containerIdx = i;
                                best.containerBranch = 1;
                                return best;
                            }
                        }
                    }
                }
            }
        }

        int botY = blocks[i].y + blockTotalHeight(blocks, i);
        if (abs(my - botY) <= snap_area && abs(mx - blocks[i].x) <= 40) {
            best.targetIdx = i;
            best.intoContainer = false;
            return best;
        }
    }
    return best;
}

static void propagateContainerChildren(vector<ScriptBlock> &blocks, int cIdx);

static void propagateSnap(vector<ScriptBlock> &blocks, int parentIdx) {
    const auto &parentBlock = blocks[parentIdx];
    bool pIsContainer = isContainerBlock(parentBlock.placed_block_in_canvas.block_type);
    int pBotY = parentBlock.y + (pIsContainer ? blockTotalHeight(blocks, parentIdx) : block_height);

    for (int i = 0; i < (int)blocks.size(); i++) {
        if (blocks[i].snapParent == parentIdx && blocks[i].container_Parent == -1) {
            blocks[i].x = parentBlock.x;
            blocks[i].y = pBotY;
            propagateSnap(blocks, i);
        }
    }

    if (pIsContainer) propagateContainerChildren(blocks, parentIdx);
}

static void propagateContainerChildren(vector<ScriptBlock> &blocks, int cIdx) {
    auto &containerBlock = blocks[cIdx];
    bool hasElse = hasElseBranch(containerBlock.placed_block_in_canvas.block_type);

    int curY0 = containerBlock.y + block_height;
    bool foundRoot0 = false;
    for (int j = 0; j < (int)blocks.size(); j++) {
        if (blocks[j].container_Parent == cIdx && blocks[j].container_Branch == 0
            && blocks[j].snapParent == -1) {
            blocks[j].x = containerBlock.x + CONTAINER_INDENT;
            blocks[j].y = curY0;
            foundRoot0 = true;

            int current_branch0_chain_index = j;
            while (current_branch0_chain_index != -1) {
                int next_branch0_snap_index = -1;
                for (int m = 0; m < (int)blocks.size(); m++)
                    if (blocks[m].snapParent == current_branch0_chain_index && blocks[m].container_Parent == cIdx
                        && blocks[m].container_Branch == 0) { next_branch0_snap_index = m; break; }
                if (next_branch0_snap_index != -1) {
                    blocks[next_branch0_snap_index].x = blocks[current_branch0_chain_index].x;
                    blocks[next_branch0_snap_index].y = blocks[current_branch0_chain_index].y + blockTotalHeight(blocks, current_branch0_chain_index);
                }
                propagateSnap(blocks, current_branch0_chain_index);
                current_branch0_chain_index = next_branch0_snap_index;
            }
        }
    }

    if (!hasElse) return;

    int innerH0 = 0;
    for (int j = 0; j < (int)blocks.size(); j++) {
        if (blocks[j].container_Parent == cIdx && blocks[j].container_Branch == 0
            && blocks[j].snapParent == -1) {
            for (int k = j; k != -1; ) {
                innerH0 += blockTotalHeight(blocks, k);
                int next = -1;
                for (int m = 0; m < (int)blocks.size(); m++)
                    if (blocks[m].snapParent == k && blocks[m].container_Parent == cIdx
                        && blocks[m].container_Branch == 0) { next = m; break; }
                k = next;
            }
        }
    }
    if (innerH0 < CONTAINER_MIN_INNER_H) innerH0 = CONTAINER_MIN_INNER_H;

    int curY1 = containerBlock.y + block_height + innerH0 + CONTAINER_BOT_H;
    for (int j = 0; j < (int)blocks.size(); j++) {
        if (blocks[j].container_Parent == cIdx && blocks[j].container_Branch == 1
            && blocks[j].snapParent == -1) {
            blocks[j].x = containerBlock.x + CONTAINER_INDENT;
            blocks[j].y = curY1;

            int current_branch1_chain_index = j;
            while (current_branch1_chain_index != -1) {
                int next_branch1_snap_index = -1;
                for (int m = 0; m < (int)blocks.size(); m++)
                    if (blocks[m].snapParent == current_branch1_chain_index && blocks[m].container_Parent == cIdx
                        && blocks[m].container_Branch == 1) { next_branch1_snap_index = m; break; }
                if (next_branch1_snap_index != -1) {
                    blocks[next_branch1_snap_index].x = blocks[current_branch1_chain_index].x;
                    blocks[next_branch1_snap_index].y = blocks[current_branch1_chain_index].y + blockTotalHeight(blocks, current_branch1_chain_index);
                }
                propagateSnap(blocks, current_branch1_chain_index);
                current_branch1_chain_index = next_branch1_snap_index;
            }
        }
    }
}

static vector<vector<Blocks_In_IDE>> buildPalette() {
    vector<vector<Blocks_In_IDE>> placing_tabs (Placing_Tab_COUNT);

    SDL_Color motion_blocks_color={70,130,180,255};
    placing_tabs[Placing_Tab_MOTION]={
        {BlockType::Move,               "Move (  ) steps",           motion_blocks_color,{}},
        {BlockType::TurnClockwise,      "Turn ClockWise (  )",        motion_blocks_color,{}},
        {BlockType::TurnAnticlockwise,  "Turn CounterClockWise (  )",       motion_blocks_color,{}},
        {BlockType::PointInDirection,   "Point direction (  )",    motion_blocks_color,{}},
        {BlockType::GoToXY,             "Go to X:(  ) Y:(  )",       motion_blocks_color,{}},
        {BlockType::ChangeXBy,          "Change X by (  )",          motion_blocks_color,{}},
        {BlockType::ChangeYBy,          "Change Y by (  )",          motion_blocks_color,{}},
        {BlockType::GoToMousePointer,   "Go to mouse pointer location",      motion_blocks_color,{}},
        {BlockType::GoToRandomPosition, "Go to random position",    motion_blocks_color,{}},
        {BlockType::IfOnEdgeBounce,     "bounce if coliding with the edge",        motion_blocks_color,{}},
    };
    SDL_Color looks_blocks_color={148,103,189,255};
    placing_tabs[Placing_Tab_LOOKS]={
        {BlockType::Show,               "Show",                     looks_blocks_color,{}},
        {BlockType::Hide,               "Hide",                     looks_blocks_color,{}},
        {BlockType::NextCostume,        "Next costume",             looks_blocks_color,{}},
        {BlockType::SetSizeTo,          "Set size to (  )%",       looks_blocks_color,{}},
        {BlockType::ChangeSizeBy,       "Change size by (  )",       looks_blocks_color,{}},
        {BlockType::ClearGraphicEffects,"Clear graphic effects",    looks_blocks_color,{}},
        {BlockType::SetColorEffectTo,   "Set color effect (  )",     looks_blocks_color,{}},
        {BlockType::ChangeColorEffectBy,"Change color effect (  )",  looks_blocks_color,{}},
        {BlockType::NextBackdrop,       "Next backdrop",            looks_blocks_color,{}},
        {BlockType::Say,                "Say (  )",               looks_blocks_color,{}},
        {BlockType::Think,              "Think (  )",             looks_blocks_color,{}},
        {BlockType::SayForSeconds,      "Say (  ) for (  ) secs",  looks_blocks_color,{}},
        {BlockType::ThinkForSeconds,    "Think (  ) for (  ) secs",looks_blocks_color,{}},
    };
    SDL_Color sound_blocks_color={220,105,132,255};
    placing_tabs[Placing_Tab_SOUND]={
        {BlockType::StartSound,         "Start sound (name)",       sound_blocks_color,{}},
        {BlockType::PlaySoundUntilDone, "Play sound until done",    sound_blocks_color,{}},
        {BlockType::StopAllSounds,      "Stop all sounds",          sound_blocks_color,{}},
        {BlockType::SetVolumeTo,        "Set volume to (  )%",     sound_blocks_color,{}},
        {BlockType::ChangeVolumeBy,     "Change volume by (  )",     sound_blocks_color,{}},
    };
    SDL_Color control_blocks_color={255,171,25,255};
    placing_tabs[Placing_Tab_CONTROL]={
        {BlockType::WhenGreenFlagClicked,"When green_button clicked",          {50,180,50,255},{}},
        {BlockType::Wait,               "Wait (  ) seconds",      control_blocks_color,{}},
        {BlockType::Repeat,             "Repeat (  )",               control_blocks_color,{}},
        {BlockType::Forever,            "Forever",                  control_blocks_color,{}},
        {BlockType::If_Then,            "If (  ) then",           control_blocks_color,{}},
        {BlockType::If_Then_Else,       "If (  ) then else",      control_blocks_color,{}},
        {BlockType::WaitUntil,          "Wait until (  )",        control_blocks_color,{}},
        {BlockType::RepeatUntil,        "Repeat until (  )",      control_blocks_color,{}},
        {BlockType::StopAll,            "Stop all",                 control_blocks_color,{}},
    };
    SDL_Color sensing_blocks_color={92,177,214,255};
    placing_tabs[Placing_Tab_SENSING]={
        {BlockType::TouchingMousePointer,"Touching mouse?",         sensing_blocks_color,{}},
        {BlockType::TouchingEdge,        "Touching edge?",          sensing_blocks_color,{}},
        {BlockType::TouchingSprite,      "Touching sprite?",        sensing_blocks_color,{}},
        {BlockType::DistanceToMouse,     "Distance to mouse",       sensing_blocks_color,{}},
        {BlockType::DistanceToSprite,    "Distance to sprite",      sensing_blocks_color,{}},
        {BlockType::MouseX,              "Mouse X",                 sensing_blocks_color,{}},
        {BlockType::MouseY,              "Mouse Y",                 sensing_blocks_color,{}},
        {BlockType::MouseDown,           "Mouse down?",             sensing_blocks_color,{}},
        {BlockType::KeyPressed,          "Key (  ) pressed?",      sensing_blocks_color,{}},
        {BlockType::Timer,               "Timer",                   sensing_blocks_color,{}},
        {BlockType::ResetTimer,          "Reset timer",             sensing_blocks_color,{}},
    };
    SDL_Color operators_blocks_color={89,189,89,255};
    placing_tabs[Placing_Tab_OPERATORS]={
        {BlockType::Addition,       "(a) + (b)",        operators_blocks_color,{}},
        {BlockType::Subtraction,    "(a) - (b)",        operators_blocks_color,{}},
        {BlockType::Multiplication, "(a) * (b)",        operators_blocks_color,{}},
        {BlockType::Division,       "(a) / (b)",        operators_blocks_color,{}},
        {BlockType::Modulos,        "(a) mod (b)",      operators_blocks_color,{}},
        {BlockType::IsEqual,        "(a) = (b)",        operators_blocks_color,{}},
        {BlockType::IsGreaterThan,  "(a) > (b)",        operators_blocks_color,{}},
        {BlockType::IsLessThan,     "(a) < (b)",        operators_blocks_color,{}},
        {BlockType::MyAbs,          "abs (  )",          operators_blocks_color,{}},
        {BlockType::MySqrt,         "sqrt (  )",         operators_blocks_color,{}},
        {BlockType::MyAnd,          "(a) and (b)",      operators_blocks_color,{}},
        {BlockType::MyOr,           "(a) or (b)",       operators_blocks_color,{}},
        {BlockType::MyNot,          "not (a)",          operators_blocks_color,{}},
        {BlockType::LengthOfString, "length of (str)",  operators_blocks_color,{}},
        {BlockType::StringConcat,   "join (s1) (s2)",   operators_blocks_color,{}},
    };
    SDL_Color variable_blocks_color={255,140,0,255};
    placing_tabs[Placing_Tab_VARIABLES]={
        {BlockType::CreateVariable,      "Create variable (name)",  variable_blocks_color,{}},
        {BlockType::SetVariableValue,    "Set (var) to (val)",      variable_blocks_color,{}},
        {BlockType::ChangeVariableValue, "Change (var) by (val)",   variable_blocks_color,{}},
        {BlockType::ShowVariable,        "Show variable (name)",    variable_blocks_color,{}},
        {BlockType::HideVariable,        "Hide variable (name)",    variable_blocks_color,{}},
        {BlockType::GetVariableValue,    "Get variable (name)",     variable_blocks_color,{}},
    };
    return placing_tabs;
}

static const char *placing_tabs_names[Placing_Tab_COUNT] = {
    "Motion","Looks","Sound","Control","Sensing","Operators","Vars"};
static SDL_Color placing_tab_colors[Placing_Tab_COUNT] = {
    {70,130,180,255},{148,103,189,255},{220,105,132,255},
    {255,171,25,255},{92,177,214,255},{89,189,89,255},{255,140,0,255}};

struct UIRects {
    SDL_Rect Placing_ettePanel, Placing_BlockArea, Placing_TabButtons[Placing_Tab_COUNT];
    SDL_Rect middlePanel,  scriptCanvas, scriptButton, costumeButton, soundButton;
    SDL_Rect stagePanel,   stageView,   greenFlagButton, stopButton;
    SDL_Rect spriteListPanel, addSpriteButton;
    SDL_Rect saveButton, loadButton;
};

static UIRects buildRects() {
    UIRects UIs_to_render;
    UIs_to_render.Placing_ettePanel = {0,0,shop_screen_width,window_screen_height};
    int tab_buttons_width=shop_screen_width/2, tab_buttons_height=40;
    for (int i=0;i<Placing_Tab_COUNT;i++) UIs_to_render.Placing_TabButtons[i]={(i%2)*tab_buttons_width,(i/2)*tab_buttons_height,tab_buttons_width,tab_buttons_height};
    int tabBarHeight = ((Placing_Tab_COUNT+1)/2)*tab_buttons_height;
    UIs_to_render.Placing_BlockArea = {0,tabBarHeight,shop_screen_width,window_screen_height-tabBarHeight};

    UIs_to_render.middlePanel  = {shop_screen_width,0,placing_screen_width,window_screen_height};
    int placing_tab_button_width=placing_screen_width/3;
    UIs_to_render.scriptButton    = {shop_screen_width,      0,placing_tab_button_width,50};
    UIs_to_render.costumeButton   = {shop_screen_width+placing_tab_button_width,   0,placing_tab_button_width,50};
    UIs_to_render.soundButton     = {shop_screen_width+2*placing_tab_button_width, 0,placing_tab_button_width,50};
    UIs_to_render.scriptCanvas = {shop_screen_width,50,placing_screen_width,window_screen_height-50};

    UIs_to_render.stagePanel      = {stage_starting_x,0,satge_width,window_screen_height};
    UIs_to_render.greenFlagButton    = {stage_starting_x+10,10,40,40};
    UIs_to_render.stopButton         = {stage_starting_x+60,10,40,40};

    UIs_to_render.saveButton         = {stage_starting_x+120,10,80,40};
    UIs_to_render.loadButton         = {stage_starting_x+210,10,80,40};
    UIs_to_render.stageView       = {stage_starting_x,60,satge_width,window_screen_height-160};
    UIs_to_render.spriteListPanel = {stage_starting_x,window_screen_height-100,satge_width,100};
    UIs_to_render.addSpriteButton    = {stage_starting_x+10,window_screen_height-90,80,40};
    return UIs_to_render;
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
    int Placing_Tab       = Placing_Tab_MOTION;
    int placeTab     = Scripts_Tab;
    int  activeSprite       = 0;
    bool greenFlagOn        = false;
    bool running            = true;
    int  Placing_ScrollY         = 0;

    int dragSrcIdx=-1, dragOffX=0, dragOffY=0;
    int dragScriptIdx=-1, dragScriptOffX=0, dragScriptOffY=0;
    int editBlockIdx=-1, editInputIdx=-1;
    int mouseX=0, mouseY=0;

    bool  contextMenuOpen  = false;
    int   contextSpriteIdx = -1;
    int   contextMenuX     = 0;
    int   contextMenuY     = 0;

    bool  costumeCtx_Open   = false;
    int   selected_costume_id    = -1;
    int   costumeCtx_X      = 0;
    int   costumeCtx_Y      = 0;

    string statusMsg;
    Uint32 statusMsgTime = 0;
};

static string runTextInputDialog(SDL_Renderer *renderer, const string &prompt) {
    string result;
    bool done  = false;
    bool ok    = false;
    SDL_StartTextInput();

    while (!done) {
        SDL_Event dialogEvent;
        while (SDL_PollEvent(&dialogEvent)) {
            switch (dialogEvent.type) {
                case SDL_QUIT:
                    done = true;
                    break;

                case SDL_KEYDOWN:
                    switch (dialogEvent.key.keysym.sym) {
                        case SDLK_RETURN:
                            done = true; ok = true;
                            break;
                        case SDLK_ESCAPE:
                            done = true;
                            break;
                        case SDLK_BACKSPACE:
                            if (!result.empty()) result.pop_back();
                            break;
                        default: break;
                    }
                    break;

                case SDL_TEXTINPUT:
                    result += dialogEvent.text.text;
                    break;

                default: break;
            }
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect overlay = {0, 0, window_screen_width, window_screen_height};
        SDL_RenderFillRect(renderer, &overlay);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Rect box = {window_screen_width/2 - 280, window_screen_height/2 - 60, 560, 120};
        SDL_SetRenderDrawColor(renderer, 50, 50, 65, 255);
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 120, 160, 220, 255);
        SDL_RenderDrawRect(renderer, &box);

        renderText(renderer, FontSmall, prompt, box.x + 14, box.y + 14);

        SDL_Rect inputBox = {box.x + 14, box.y + 44, box.w - 28, 30};
        SDL_SetRenderDrawColor(renderer, 240, 240, 255, 255);
        SDL_RenderFillRect(renderer, &inputBox);
        SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
        SDL_RenderDrawRect(renderer, &inputBox);
        renderText(renderer, FontSmall, result, inputBox.x + 6, inputBox.y + 7, {20, 20, 20, 255});

        if ((SDL_GetTicks() / 500) % 2 == 0) {
            int cursorX = inputBox.x + 6 + textWidth(FontSmall, result);
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
            SDL_RenderDrawLine(renderer, cursorX, inputBox.y + 4, cursorX, inputBox.y + 26);
        }

        renderText(renderer, FontSmall, "Enter to confirm  |  Esc to cancel",
                   box.x + 14, box.y + 86, {160, 160, 180, 255});

        SDL_RenderPresent(renderer);
    }
    return ok ? result : "";
}

static SDL_Texture *launchCostumeEditor(SDL_Renderer *renderer, const string &savePath) {
    {
        ifstream test(savePath);
        if (!test.good()) {
            constexpr int W = 480, H = 360;
            SDL_Surface *blank = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32,
                                     SDL_PIXELFORMAT_RGBA8888);
            if (blank) {
                SDL_FillRect(blank, nullptr,
                             SDL_MapRGBA(blank->format, 255, 255, 255, 0));
                if (IMG_SavePNG(blank, savePath.c_str()) != 0)
                    cout << "[costume_editor] could not write blank canvas: "
                         << IMG_GetError() << "\n";
                SDL_FreeSurface(blank);
            }
        }
    }

#ifdef _WIN32
    string cmd = "costume_editor.exe \"" + savePath + "\"";
#else
    string cmd = "./costume_editor \"" + savePath + "\"";
#endif
    int ret = system(cmd.c_str());
    if (ret != 0)
        cout << "[costume_editor] exited with code " << ret << "\n";

    SDL_Texture *tex = IMG_LoadTexture(renderer, savePath.c_str());
    if (!tex)
        cout << "[costume_editor] could not load saved image: "
             << IMG_GetError() << "\n";
    return tex;
}

static bool promptAndLoadSound(SDL_Renderer *renderer, SpriteEntry &new_sprite) {
    string filePath = runTextInputDialog(renderer,
        "Sound file path (WAV or MP3, e.g. ./sounds/jump.wav):");
    if (filePath.empty()) return false;

    string soundName = runTextInputDialog(renderer,
        "Sound name / alias (used in blocks, e.g. \"jump\"):");
    if (soundName.empty()) return false;

    {
        ifstream test(filePath);
        if (!test.good()) {
            cout << "[sound] file not found: " << filePath << "\n";
            return false;
        }
    }

    addSound(new_sprite.sprite, filePath, soundName);
    new_sprite.soundNames.push_back(soundName);
    new_sprite.soundFilePaths.push_back(filePath);
    return true;
}

static string escapeLine(const string &s) {
    string out;
    for (char c : s) { if (c=='\\') out+="\\\\"; else if (c=='\n') out+="\\n"; else out+=c; }
    return out;
}
static string unescapeLine(const string &s) {
    string out;
    for (size_t i=0; i<s.size(); i++) {
        if (s[i]=='\\' && i+1<s.size()) {
            if (s[i+1]=='n') { out+='\n'; i++; }
            else if (s[i+1]=='\\') { out+='\\'; i++; }
            else out+=s[i];
        } else out+=s[i];
    }
    return out;
}

static Blocks_In_IDE findProto(const vector<vector<Blocks_In_IDE>> &Placing_ette, int btypeInt) {
    for (auto &cat : Placing_ette)
        for (auto &b : cat)
            if (static_cast<int>(b.block_type) == btypeInt) return b;
    Blocks_In_IDE dummy;
    dummy.block_type = static_cast<BlockType>(btypeInt);
    dummy.label = "Unknown";
    dummy.color = {180,180,180,255};
    return dummy;
}

static bool saveLayout(const vector<ScriptBlock> &blocks,
                       const vector<SpriteEntry>  &sprites,
                       const string &filepath) {
    ofstream f(filepath);
    if (!f) return false;

    f << "SCRATCH_SAVE_V2\n";

    f << "SPRITE_COUNT " << sprites.size() << "\n";
    for (auto &new_sprite : sprites) {
        f << "SPRITE_NAME "  << escapeLine(new_sprite.name) << "\n";
        f << "SPRITE_POS "   << new_sprite.sprite.xCenter << " " << new_sprite.sprite.yCenter << "\n";
        f << "SPRITE_SIZE "  << new_sprite.sprite.spriteSize << "\n";
        f << "SPRITE_DIR "   << new_sprite.sprite.direction  << "\n";
        int costumeCount = (int)new_sprite.sprite.spriteCostumes.size();
        f << "COSTUME_COUNT " << costumeCount << "\n";
        f << "COSTUME_IDX "   << new_sprite.sprite.currentCostumeIndex << "\n";
        for (int i = 0; i < costumeCount; i++) {
            string p = (i < (int)new_sprite.costumePaths.size()) ? new_sprite.costumePaths[i] : "";
            f << "COSTUME_PATH " << escapeLine(p) << "\n";
        }
        int soundCount = (int)new_sprite.soundNames.size();
        f << "SOUND_COUNT " << soundCount << "\n";
        for (int i = 0; i < soundCount; i++) {
            f << "SOUND_ALIAS " << escapeLine(new_sprite.soundNames[i]) << "\n";
            string fp = (i < (int)new_sprite.soundFilePaths.size()) ? new_sprite.soundFilePaths[i] : "";
            f << "SOUND_FILE "  << escapeLine(fp) << "\n";
        }
    }

    f << "BLOCK_COUNT " << blocks.size() << "\n";
    for (auto &scriptBlock : blocks) {
        f << "BTYPE "  << static_cast<int>(scriptBlock.placed_block_in_canvas.block_type) << "\n";
        f << "LABEL "  << escapeLine(scriptBlock.placed_block_in_canvas.label) << "\n";
        f << "COLOR "  << (int)scriptBlock.placed_block_in_canvas.color.r << " "
                       << (int)scriptBlock.placed_block_in_canvas.color.g << " "
                       << (int)scriptBlock.placed_block_in_canvas.color.b << " "
                       << (int)scriptBlock.placed_block_in_canvas.color.a << "\n";
        f << "POS "    << scriptBlock.x << " " << scriptBlock.y << "\n";
        f << "SNAP "   << scriptBlock.snapParent << "\n";
        f << "CPAR "   << scriptBlock.container_Parent << "\n";
        f << "CBRN "   << scriptBlock.container_Branch << "\n";
        f << "INPUTS " << scriptBlock.input_Values.size() << "\n";
        for (auto &v : scriptBlock.input_Values)
            f << "INPUT " << escapeLine(v) << "\n";
    }
    return true;
}

static string readVal(ifstream &f, const string &keyword) {
    string line;
    if (!getline(f, line)) return "";
    string prefix = keyword + " ";
    if (line.size() >= prefix.size() && line.substr(0, prefix.size()) == prefix)
        return unescapeLine(line.substr(prefix.size()));
    if (line == keyword) return "";
    return unescapeLine(line);
}
static int readInt(ifstream &f, const string &keyword) {
    string s = readVal(f, keyword);
    if (s.empty()) return 0;
    try { return stoi(s); } catch (...) { return 0; }
}

static bool loadLayout(vector<ScriptBlock>         &blocks,
                       vector<SpriteEntry>          &sprites,
                       const vector<vector<Blocks_In_IDE>> &Placing_ette,
                       SDL_Renderer                 *renderer,
                       const string                 &filepath) {
    ifstream f(filepath);
    if (!f) return false;

    string header;
    if (!getline(f, header)) return false;
    bool v2 = (header == "SCRATCH_SAVE_V2");

    if (v2) {
        for (auto &new_sprite : sprites) freeAllSounds(new_sprite.sprite);
        sprites.clear();

        int sprite_count = readInt(f, "SPRITE_COUNT");
        for (int s = 0; s < sprite_count; s++) {
            SpriteEntry new_sprite;
            new_sprite.name = readVal(f, "SPRITE_NAME");

            double px = 0, py = 0;
            { string pl = readVal(f, "SPRITE_POS"); sscanf(pl.c_str(), "%lf %lf", &px, &py); }
            double sz = 0, dir = 0;
            { string sl = readVal(f, "SPRITE_SIZE"); try { sz  = stod(sl); } catch (...) {} }
            { string dl = readVal(f, "SPRITE_DIR");  try { dir = stod(dl); } catch (...) {} }

            int costumeCount = readInt(f, "COSTUME_COUNT");
            int costumeIdx   = readInt(f, "COSTUME_IDX");

            for (int c = 0; c < costumeCount; c++) {
                string path = readVal(f, "COSTUME_PATH");
                new_sprite.costumePaths.push_back(path);

                SDL_Texture *tex = nullptr;
                if (!path.empty())
                    tex = IMG_LoadTexture(renderer, path.c_str());

                if (!tex) {

                    tex = SDL_CreateTexture(renderer,
                        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
                    Uint32 color = (c == 0) ? 0xFFFFFFFF : 0xFF6464FF;
                    SDL_UpdateTexture(tex, nullptr, &color, 4);
                    path = "";
                }

                string costumeName = path.empty() ? "default" : path;
                if (c == 0)
                    new_sprite.sprite = createSprite(tex, costumeName,
                                             stage_starting_x*2 + satge_width,
                                             (60)*2 + (window_screen_height-160));
                else
                    addCostume(new_sprite.sprite, tex, costumeName);
            }

            if (costumeCount == 0) {
                SDL_Texture *placeholderTexture = SDL_CreateTexture(renderer,
                    SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
                Uint32 white = 0xFFFFFFFF;
                SDL_UpdateTexture(placeholderTexture, nullptr, &white, 4);
                new_sprite.sprite = createSprite(placeholderTexture, "default",
                                         stage_starting_x*2 + satge_width,
                                         (60)*2 + (window_screen_height-160));
                new_sprite.costumePaths.push_back("");
            }

            new_sprite.sprite.xCenter    = px;
            new_sprite.sprite.yCenter    = py;
            new_sprite.sprite.spriteSize = sz;
            new_sprite.sprite.direction  = dir;
            if (costumeIdx >= 0 && costumeIdx < (int)new_sprite.sprite.spriteCostumes.size())
                new_sprite.sprite.currentCostumeIndex = costumeIdx;
            new_sprite.thumbnail = new_sprite.sprite.spriteCostumes[new_sprite.sprite.currentCostumeIndex].costumeTexture;

            int soundCount = readInt(f, "SOUND_COUNT");
            for (int i = 0; i < soundCount; i++) {
                string alias = readVal(f, "SOUND_ALIAS");
                string file    = readVal(f, "SOUND_FILE");
                if (!file.empty()) {
                    ifstream test(file);
                    if (test.good()) {
                        addSound(new_sprite.sprite, file, alias);
                    } else {
                        cout << "[load] sound file not found: " << file << "\n";
                        alias += " (missing)";
                    }
                }
                new_sprite.soundNames.push_back(alias);
                new_sprite.soundFilePaths.push_back(file);
            }

            sprites.push_back(move(new_sprite));
        }

        if (sprites.empty()) {
            SpriteEntry new_sprite; new_sprite.name = "Sprite1";
            SDL_Texture *placeholderTexture = SDL_CreateTexture(renderer,
                SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
            Uint32 white = 0xFFFFFFFF;
            SDL_UpdateTexture(placeholderTexture, nullptr, &white, 4);
            new_sprite.sprite = createSprite(placeholderTexture, "default",
                                     stage_starting_x*2 + satge_width,
                                     (60)*2 + (window_screen_height-160));
            new_sprite.thumbnail = placeholderTexture;
            new_sprite.costumePaths.push_back("");
            sprites.push_back(new_sprite);
        }
    }

    string blockHeader = v2 ? "" : header;
    if (v2 && !getline(f, blockHeader)) return true;

    if (blockHeader.substr(0, 12) != "BLOCK_COUNT ") return true;
    int count = 0;
    try { count = stoi(blockHeader.substr(12)); } catch (...) { return true; }

    blocks.clear();
    for (int i = 0; i < count; i++) {
        ScriptBlock loadedBlock;

        int btypeInt = readInt(f, "BTYPE");
        loadedBlock.placed_block_in_canvas = findProto(Placing_ette, btypeInt);
        loadedBlock.placed_block_in_canvas.label = readVal(f, "LABEL");

        {
            string cl = readVal(f, "COLOR");
            int cr=180,cg=180,cb=180,ca=255;
            sscanf(cl.c_str(), "%d %d %d %d", &cr, &cg, &cb, &ca);
            loadedBlock.placed_block_in_canvas.color = { (Uint8)cr, (Uint8)cg, (Uint8)cb, (Uint8)ca };
        }
        {
            string pl = readVal(f, "POS");
            sscanf(pl.c_str(), "%d %d", &loadedBlock.x, &loadedBlock.y);
        }

        loadedBlock.snapParent      = readInt(f, "SNAP");
        loadedBlock.container_Parent = readInt(f, "CPAR");
        loadedBlock.container_Branch = readInt(f, "CBRN");

        int ic = readInt(f, "INPUTS");
        loadedBlock.input_Values.resize(ic);
        for (int j = 0; j < ic; j++)
            loadedBlock.input_Values[j] = readVal(f, "INPUT");

        loadedBlock.active_Input = -1;
        blocks.push_back(move(loadedBlock));
    }
    return true;
}

static vector<Block*> rootBlocks;

static Block* makeBlockFromScriptBlock(const ScriptBlock &scriptBlock, Sprite &spr,
                                       SDL_Renderer *renderer, TTF_Font *font) {
    Block *b = new Block();
    b->type = scriptBlock.placed_block_in_canvas.block_type;

    auto numAt = [&](int idx) -> double {
        if (idx < (int)scriptBlock.input_Values.size() && !scriptBlock.input_Values[idx].empty()) {
            try { return stod(scriptBlock.input_Values[idx]); } catch (...) {}
        }
        return 0.0;
    };
    auto strAt = [&](int idx) -> string {
        if (idx < (int)scriptBlock.input_Values.size()) return scriptBlock.input_Values[idx];
        return "";
    };

    auto spriteValue = [&]() -> Value {
        Value v; v.type = DataType::SPRITE; v.sprPtr = &spr; return v;
    };
    auto numberValue = [&](double n) -> Value {
        Value v; v.type = DataType::NUMBER; v.numVal = n; return v;
    };
    auto stringValue = [&](const string &s) -> Value {
        Value v; v.type = DataType::STRING; v.strVal = s; return v;
    };
    auto boolVal = [&](bool booleanValue) -> Value {
        Value v; v.type = DataType::BOOLEAN; v.boolVal = booleanValue; return v;
    };
    auto rndVal = [&]() -> Value {
        Value v; v.type = DataType::SDL_RENDERER; v.rndVal = renderer; return v;
    };
    auto fntVal = [&]() -> Value {
        Value v; v.type = DataType::TTF_FONT; v.fntVal = font; return v;
    };

    switch (b->type) {

        case BlockType::Move:
        case BlockType::TurnClockwise:
        case BlockType::TurnAnticlockwise:
        case BlockType::PointInDirection:
        case BlockType::ChangeXBy:
        case BlockType::ChangeYBy:
            b->parameters = { spriteValue(), numberValue(numAt(0)) }; break;
        case BlockType::GoToXY:
            b->parameters = { spriteValue(), numberValue(numAt(0)), numberValue(numAt(1)) }; break;
        case BlockType::GoToMousePointer:
        case BlockType::GoToRandomPosition:
        case BlockType::IfOnEdgeBounce:
            b->parameters = { spriteValue() }; break;

        case BlockType::Show:
        case BlockType::Hide:
        case BlockType::NextCostume:
        case BlockType::ClearGraphicEffects:
            b->parameters = { spriteValue() }; break;
        case BlockType::SetSizeTo:
        case BlockType::ChangeSizeBy:
        case BlockType::SetColorEffectTo:
        case BlockType::ChangeColorEffectBy:
            b->parameters = { spriteValue(), numberValue(numAt(0)) }; break;
        case BlockType::SwitchCostumeTo:
            b->parameters = { spriteValue(), stringValue(strAt(0)) }; break;
        case BlockType::Say:
        case BlockType::Think:
            b->parameters = { spriteValue(), stringValue(strAt(0)), rndVal(), fntVal() }; break;
        case BlockType::SayForSeconds:
        case BlockType::ThinkForSeconds:
            b->parameters = { spriteValue(), stringValue(strAt(0)), rndVal(), fntVal(), numberValue(numAt(1)) }; break;

        case BlockType::StartSound:
        case BlockType::PlaySoundUntilDone:
            b->parameters = { spriteValue(), stringValue(strAt(0)) }; break;
        case BlockType::StopAllSounds:
            b->parameters = { spriteValue() }; break;
        case BlockType::SetVolumeTo:
        case BlockType::ChangeVolumeBy:
            b->parameters = { spriteValue(), stringValue(strAt(0)), numberValue(numAt(1)) }; break;

        case BlockType::WhenGreenFlagClicked:
        case BlockType::WhenSomeKeyPressed:
        case BlockType::WhenThisSpriteClicked:
        case BlockType::WhenXIsGreaterThanY:
        case BlockType::Forever:
        case BlockType::StopAll:
            break;
        case BlockType::Wait:
            b->parameters = { numberValue(numAt(0) * 1000) }; break;
        case BlockType::Repeat:
            b->parameters = { numberValue(numAt(0)) }; break;
        case BlockType::If_Then:
        case BlockType::If_Then_Else:
        case BlockType::WaitUntil:
        case BlockType::RepeatUntil:
            b->parameters = { boolVal(false) }; break;

        case BlockType::TouchingMousePointer:
        case BlockType::TouchingEdge:
        case BlockType::DistanceToMouse:
        case BlockType::SetDragMode:
            b->parameters = { spriteValue() }; break;
        case BlockType::KeyPressed:
            b->parameters = { stringValue(strAt(0)) }; break;
        case BlockType::ResetTimer:
        case BlockType::Timer:
        case BlockType::MouseX:
        case BlockType::MouseY:
        case BlockType::MouseDown:
            break;

        case BlockType::Addition:
        case BlockType::Subtraction:
        case BlockType::Multiplication:
        case BlockType::Division:
        case BlockType::Modulos:
        case BlockType::IsEqual:
        case BlockType::IsGreaterThan:
        case BlockType::IsLessThan:
        case BlockType::MyAnd:
        case BlockType::MyOr:
        case BlockType::MyXor:
            b->parameters = { numberValue(numAt(0)), numberValue(numAt(1)) }; break;
        case BlockType::MyAbs:
        case BlockType::MySqrt:
        case BlockType::MyFloor:
        case BlockType::MyCeil:
        case BlockType::MySinus:
        case BlockType::MyCosine:
        case BlockType::MyNot:
        case BlockType::LengthOfString:
            b->parameters = { numberValue(numAt(0)) }; break;
        case BlockType::StringConcat:
            b->parameters = { stringValue(strAt(0)), stringValue(strAt(1)) }; break;
        case BlockType::CharAt:
            b->parameters = { numberValue(numAt(0)), stringValue(strAt(1)) }; break;

        case BlockType::CreateVariable:
        case BlockType::ShowVariable:
        case BlockType::HideVariable:
        case BlockType::GetVariableValue:
            b->parameters = { stringValue(strAt(0)), numberValue(0) }; break;
        case BlockType::SetVariableValue:
        case BlockType::ChangeVariableValue: {
            Value variableValue; variableValue.type = DataType::NUMBER; variableValue.numVal = numAt(1);
            b->parameters = { stringValue(strAt(0)), numberValue(0), variableValue }; break;
        }
        default: break;
    }
    return b;
}

static void buildCodeSpace(const vector<ScriptBlock> &scriptBlocks,
                           vector<SpriteEntry> &sprites, int spriteIdx,
                           SDL_Renderer *renderer, TTF_Font *font) {
    for (auto *b : codeSpace) delete b;
    codeSpace.clear();
    rootBlocks.clear();
    for (auto *interp : multiInterpreter.interpreters) delete interp;
    multiInterpreter.interpreters.clear();
    multiInterpreter.events.clear();

    if (sprites.empty() || spriteIdx < 0 || spriteIdx >= (int)sprites.size()) return;
    Sprite &spr = sprites[spriteIdx].sprite;

    vector<Block*> built(scriptBlocks.size(), nullptr);
    for (int i = 0; i < (int)scriptBlocks.size(); i++)
        built[i] = makeBlockFromScriptBlock(scriptBlocks[i], spr, renderer, font);

    for (int i = 0; i < (int)scriptBlocks.size(); i++) {
        const auto &currentScriptBlock = scriptBlocks[i];

        for (int j = 0; j < (int)scriptBlocks.size(); j++) {
            if (i == j) continue;
            if (scriptBlocks[j].snapParent == i) {
                if (currentScriptBlock.container_Parent < 0) {

                    built[i]->isConnectedTo = built[j];
                }

                break;
            }
        }
    }

    for (int i = 0; i < (int)scriptBlocks.size(); i++) {
        if (!isContainerBlock(scriptBlocks[i].placed_block_in_canvas.block_type)) continue;

        auto collectBranch = [&](int branch, vector<unique_ptr<Block>> &dest) {

            int root = -1;
            for (int j = 0; j < (int)scriptBlocks.size(); j++) {
                if (scriptBlocks[j].container_Parent == i &&
                    scriptBlocks[j].container_Branch == branch &&
                    scriptBlocks[j].snapParent == -1) {
                    root = j; break;
                }
            }

            for (int k = root; k != -1; ) {

                int next = -1;
                for (int m = 0; m < (int)scriptBlocks.size(); m++) {
                    if (scriptBlocks[m].snapParent == k &&
                        scriptBlocks[m].container_Parent == i &&
                        scriptBlocks[m].container_Branch == branch) {
                        next = m; break;
                    }
                }

                dest.emplace_back(built[k]);
                built[k] = nullptr;
                k = next;
            }
        };

        collectBranch(0, built[i]->children);
        if (hasElseBranch(scriptBlocks[i].placed_block_in_canvas.block_type))
            collectBranch(1, built[i]->second_children);
    }

    for (int i = 0; i < (int)scriptBlocks.size(); i++) {
        if (built[i] == nullptr) continue;
        codeSpace.push_back(built[i]);
        if (scriptBlocks[i].snapParent == -1 && scriptBlocks[i].container_Parent == -1)
            rootBlocks.push_back(built[i]);
    }

    for (Block *root : rootBlocks) {
        EventType evType = WHEN_GREEN_FLAG_CLICKED;
        bool isEvent = true;
        switch (root->type) {
            case BlockType::WhenGreenFlagClicked:  evType = WHEN_GREEN_FLAG_CLICKED;   break;
            case BlockType::WhenSomeKeyPressed:    evType = WHEN_SOME_KEY_PRESSED;     break;
            case BlockType::WhenThisSpriteClicked: evType = WHEN_THIS_SPRITE_CLICKED;  break;
            case BlockType::WhenXIsGreaterThanY:   evType = WHEN_X_IS_GREATER_THAN_Y;  break;
            default: isEvent = false; break;
        }
        Interpreter *interp = new Interpreter();
        interp->isRunning = false;
        Block *start = isEvent ? root->isConnectedTo : root;
        vector<Block*> chain;
        for (Block *b = start; b != nullptr; b = b->isConnectedTo) chain.push_back(b);
        for (int ci = (int)chain.size()-1; ci >= 0; ci--)
            interp->execStack.push_back(ExecutionFrame{chain[ci], 0});
        if (isEvent && evType == WHEN_GREEN_FLAG_CLICKED && !interp->execStack.empty())
            interp->isRunning = true;
        multiInterpreter.interpreters.push_back(interp);
        multiInterpreter.events.push_back(Event{evType});
    }
}

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
        SayBubbleTex = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        Uint32 white = 0xFFFFFFFF;
        SDL_UpdateTexture(SayBubbleTex, nullptr, &white, 4);
    }

    {
        ThinkBubbleTex = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
        Uint32 lavender = 0xE8D5FFFF;
        SDL_UpdateTexture(ThinkBubbleTex, nullptr, &lavender, 4);
    }

    setGameArea(stage_starting_x,60,satge_width,window_screen_height-160);

    auto Placing_ette=buildPalette();
    UIRects ui=buildRects();
    AppState state;
    vector<ScriptBlock> scriptBlocks;

    vector<SpriteEntry> sprites;
    {
        SpriteEntry new_sprite; new_sprite.name="Sprite1";
        SDL_Texture *placeholderTexture=SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_STATIC,1,1);
        Uint32 white=0xFFFFFFFF; SDL_UpdateTexture(placeholderTexture,nullptr,&white,4);
        new_sprite.sprite=createSprite(placeholderTexture,"default",
                               stage_starting_x*2 + satge_width,
                               (60)*2 + (window_screen_height-160));
        new_sprite.thumbnail=placeholderTexture;
        new_sprite.costumePaths.push_back("");
        sprites.push_back(new_sprite);
    }
    Stage stage; setUpStage(stage);

    SDL_StartTextInput();

    SDL_Event sdlEvent;
    while (state.running) {

        while (SDL_PollEvent(&sdlEvent)) {

            switch (sdlEvent.type) {

                case SDL_TEXTINPUT:
                    if (state.editBlockIdx>=0 && state.editInputIdx>=0) {
                        auto &scriptBlock=scriptBlocks[state.editBlockIdx];
                        if (state.editInputIdx<(int)scriptBlock.input_Values.size())
                            scriptBlock.input_Values[state.editInputIdx]+=sdlEvent.text.text;
                    }
                    break;

                case SDL_KEYDOWN:
                    switch (sdlEvent.key.keysym.sym) {
                        case SDLK_BACKSPACE:
                            if (state.editBlockIdx>=0 && state.editInputIdx>=0) {
                                auto &val=scriptBlocks[state.editBlockIdx].input_Values[state.editInputIdx];
                                if (!val.empty()) val.pop_back();
                            }
                            break;
                        case SDLK_RETURN:
                        case SDLK_ESCAPE:
                            if (state.editBlockIdx>=0)
                                scriptBlocks[state.editBlockIdx].active_Input=-1;
                            state.editBlockIdx=state.editInputIdx=-1;
                            break;
                        default: break;
                    }
                    break;

                case SDL_QUIT:
                    state.running=false;
                    break;

                case SDL_MOUSEMOTION:
                    state.mouseX=sdlEvent.motion.x; state.mouseY=sdlEvent.motion.y;
                    if (state.dragScriptIdx>=0) {
                        auto &scriptBlock=scriptBlocks[state.dragScriptIdx];
                        scriptBlock.x=state.mouseX-state.dragScriptOffX;
                        scriptBlock.y=state.mouseY-state.dragScriptOffY;
                        scriptBlock.snapParent=-1;
                        propagateSnap(scriptBlocks,state.dragScriptIdx);
                    }
                    if (!sprites.empty()) spriteDrag(sprites[state.activeSprite].sprite,sdlEvent);
                    break;

                case SDL_MOUSEWHEEL: {
                    SDL_Point mousePoint={state.mouseX,state.mouseY};
                    if (SDL_PointInRect(&mousePoint,&ui.Placing_BlockArea)) {
                        state.Placing_ScrollY-=sdlEvent.wheel.y*(block_height+blocks_gap);
                        if (state.Placing_ScrollY<0) state.Placing_ScrollY=0;
                    }
                    break;
                }

                default: break;
            }

            if (sdlEvent.type==SDL_MOUSEBUTTONDOWN && sdlEvent.button.button==SDL_BUTTON_LEFT) {
                SDL_Point mousePoint={sdlEvent.button.x,sdlEvent.button.y};

                if (state.contextMenuOpen) {
                    SDL_Rect editButton   = {state.contextMenuX,      state.contextMenuY,      180, 28};
                    SDL_Rect deleteButton = {state.contextMenuX,      state.contextMenuY + 28, 180, 28};

                    if (SDL_PointInRect(&mousePoint, &editButton)) {
                        int idx = state.contextSpriteIdx;
                        if (idx >= 0 && idx < (int)sprites.size()) {
                            auto &new_sprite = sprites[idx];
                            string savePath;
                            if (!new_sprite.sprite.spriteCostumes.empty()) {
                                const string &cn = new_sprite.sprite.spriteCostumes[
                                    new_sprite.sprite.currentCostumeIndex].costumeName;
                                if (cn.size() > 4 && cn.substr(cn.size()-4) == ".png")
                                    savePath = cn;
                            }
                            if (savePath.empty())
                                savePath = "./sprites/costume_" + new_sprite.name + ".png";
                            SDL_Texture *newTex = launchCostumeEditor(renderer, savePath);
                            if (newTex) {
                                int ci = new_sprite.sprite.currentCostumeIndex;
                                new_sprite.sprite.spriteCostumes[ci].costumeTexture = newTex;
                                new_sprite.sprite.spriteCostumes[ci].costumeName = savePath;
                                SDL_QueryTexture(newTex, nullptr, nullptr,
                                                 &new_sprite.sprite.costumeWidth, &new_sprite.sprite.costumeHeight);
                                new_sprite.thumbnail = newTex;

                                if (ci < (int)new_sprite.costumePaths.size())
                                    new_sprite.costumePaths[ci] = savePath;
                                else {
                                    new_sprite.costumePaths.resize(ci + 1, "");
                                    new_sprite.costumePaths[ci] = savePath;
                                }
                            }
                        }
                        state.contextMenuOpen = false;
                        continue;
                    }
                    else if (SDL_PointInRect(&mousePoint, &deleteButton)) {
                        int idx = state.contextSpriteIdx;
                        if (idx >= 0 && idx < (int)sprites.size()) {
                            freeAllSounds(sprites[idx].sprite);
                            sprites.erase(sprites.begin() + idx);
                            if (state.activeSprite >= (int)sprites.size())
                                state.activeSprite = max(0, (int)sprites.size()-1);
                        }
                        state.contextMenuOpen = false;
                        continue;
                    }
                    else {
                        state.contextMenuOpen = false;
                    }
                }

                if (state.costumeCtx_Open) {
                    SDL_Rect editButton   = {state.costumeCtx_X, state.costumeCtx_Y,      180, 28};
                    SDL_Rect deleteButton = {state.costumeCtx_X, state.costumeCtx_Y + 28, 180, 28};

                    if (SDL_PointInRect(&mousePoint, &editButton)) {
                        if (!sprites.empty()) {
                            auto &new_sprite = sprites[state.activeSprite];
                            int ci = state.selected_costume_id;
                            if (ci >= 0 && ci < (int)new_sprite.sprite.spriteCostumes.size()) {

                                string savePath;
                                if (ci < (int)new_sprite.costumePaths.size() && !new_sprite.costumePaths[ci].empty())
                                    savePath = new_sprite.costumePaths[ci];
                                else
                                    savePath = "./sprites/costume_" + new_sprite.name + "_" + to_string(ci) + ".png";

                                SDL_Texture *newTex = launchCostumeEditor(renderer, savePath);
                                if (newTex) {
                                    new_sprite.sprite.spriteCostumes[ci].costumeTexture = newTex;
                                    new_sprite.sprite.spriteCostumes[ci].costumeName = savePath;
                                    SDL_QueryTexture(newTex, nullptr, nullptr,
                                                     &new_sprite.sprite.costumeWidth, &new_sprite.sprite.costumeHeight);
                                    if (ci == new_sprite.sprite.currentCostumeIndex)
                                        new_sprite.thumbnail = newTex;
                                    if (ci < (int)new_sprite.costumePaths.size())
                                        new_sprite.costumePaths[ci] = savePath;
                                    else {
                                        new_sprite.costumePaths.resize(ci + 1, "");
                                        new_sprite.costumePaths[ci] = savePath;
                                    }
                                }
                            }
                        }
                        state.costumeCtx_Open = false;
                        continue;
                    }
                    else if (SDL_PointInRect(&mousePoint, &deleteButton)) {
                        if (!sprites.empty()) {
                            auto &new_sprite = sprites[state.activeSprite];
                            int ci = state.selected_costume_id;
                            int costumeCount = (int)new_sprite.sprite.spriteCostumes.size();
                            if (costumeCount > 1 && ci >= 0 && ci < costumeCount) {

                                new_sprite.sprite.spriteCostumes.erase(
                                    new_sprite.sprite.spriteCostumes.begin() + ci);
                                if (ci < (int)new_sprite.costumePaths.size())
                                    new_sprite.costumePaths.erase(new_sprite.costumePaths.begin() + ci);

                                if (new_sprite.sprite.currentCostumeIndex >= (int)new_sprite.sprite.spriteCostumes.size())
                                    new_sprite.sprite.currentCostumeIndex = (int)new_sprite.sprite.spriteCostumes.size()-1;
                                new_sprite.thumbnail = new_sprite.sprite.spriteCostumes[
                                    new_sprite.sprite.currentCostumeIndex].costumeTexture;
                                SDL_QueryTexture(new_sprite.thumbnail, nullptr, nullptr,
                                                 &new_sprite.sprite.costumeWidth, &new_sprite.sprite.costumeHeight);
                            }
                        }
                        state.costumeCtx_Open = false;
                        continue;
                    }
                    else {
                        state.costumeCtx_Open = false;
                    }
                }

                for (int i=0;i<Placing_Tab_COUNT;i++)
                    if (SDL_PointInRect(&mousePoint,&ui.Placing_TabButtons[i])){
                        state.Placing_Tab=i; state.Placing_ScrollY=0; }

                if (SDL_PointInRect(&mousePoint,&ui.scriptButton))  state.placeTab=Scripts_Tab;
                if (SDL_PointInRect(&mousePoint,&ui.costumeButton)) state.placeTab=Customs_Tab;
                if (SDL_PointInRect(&mousePoint,&ui.soundButton))   state.placeTab=Sounds_Tab;

                if (state.placeTab == Customs_Tab && !sprites.empty()) {
                    SDL_Rect changeCosButton = {ui.scriptCanvas.x + 10, ui.scriptCanvas.y + 10, 160, 34};
                    if (SDL_PointInRect(&mousePoint, &changeCosButton)) {
                        auto &new_sprite = sprites[state.activeSprite];
                        string savePath = "./sprites/costume_" + new_sprite.name + "_" +
                                          to_string(new_sprite.sprite.currentCostumeIndex) + ".png";
                        SDL_Texture *newTex = launchCostumeEditor(renderer, savePath);
                        if (newTex) {
                            addCostume(new_sprite.sprite, newTex, savePath);
                            switchCostumeTo(new_sprite.sprite, savePath);
                            new_sprite.thumbnail = newTex;
                            new_sprite.costumePaths.push_back(savePath);
                        }
                    }

                    auto &new_sprite = sprites[state.activeSprite];
                    int costumeAreaX = ui.scriptCanvas.x + 10;
                    int costumeAreaY = ui.scriptCanvas.y + 60;
                    for (int i = 0; i < (int)new_sprite.sprite.spriteCostumes.size(); i++) {
                        SDL_Rect frame = {costumeAreaX, costumeAreaY + i*90, 80, 80};
                        if (SDL_PointInRect(&mousePoint, &frame)) {
                            new_sprite.sprite.currentCostumeIndex = i;
                            new_sprite.thumbnail = new_sprite.sprite.spriteCostumes[i].costumeTexture;
                        }
                    }
                }

                if (state.placeTab == Sounds_Tab && !sprites.empty()) {
                    SDL_Rect addSoundButton = {ui.scriptCanvas.x + 10, ui.scriptCanvas.y + 10, 140, 34};
                    if (SDL_PointInRect(&mousePoint, &addSoundButton)) {
                        promptAndLoadSound(renderer, sprites[state.activeSprite]);
                    }
                }

                if (SDL_PointInRect(&mousePoint,&ui.greenFlagButton)) {
                    state.greenFlagOn = true;
                    buildCodeSpace(scriptBlocks, sprites, state.activeSprite, renderer, FontSmall);
                }

                if (SDL_PointInRect(&mousePoint,&ui.stopButton)) {
                    state.greenFlagOn = false;
                    for (auto *interp : multiInterpreter.interpreters) interp->isRunning = false;
                    Mix_HaltChannel(-1);
                    for (auto &new_sprite : sprites) {
                        new_sprite.sprite.saying = new_sprite.sprite.thinking = false;
                        new_sprite.sprite.bubbleEnabledTime = 0;
                        if (new_sprite.sprite.textTexture) { SDL_DestroyTexture(new_sprite.sprite.textTexture); new_sprite.sprite.textTexture = nullptr; }
                    }
                }

                if (SDL_PointInRect(&mousePoint, &ui.saveButton)) {
                    string filepath = runTextInputDialog(renderer, "Save layout to file (e.g. ./saves/my_layout.txt):");
                    if (!filepath.empty()) {
                        state.statusMsg = saveLayout(scriptBlocks, sprites, filepath)
                            ? "Saved: " + filepath : "Save FAILED: " + filepath;
                        state.statusMsgTime = SDL_GetTicks();
                    }
                }

                if (SDL_PointInRect(&mousePoint, &ui.loadButton)) {
                    string filepath = runTextInputDialog(renderer, "Load layout from file (e.g. ./saves/my_layout.txt):");
                    if (!filepath.empty()) {
                        if (loadLayout(scriptBlocks, sprites, Placing_ette, renderer, filepath))
                            { state.activeSprite = 0; state.statusMsg = "Loaded: " + filepath; }
                        else state.statusMsg = "Load FAILED: " + filepath;
                        state.statusMsgTime = SDL_GetTicks();
                    }
                }

                if (SDL_PointInRect(&mousePoint,&ui.addSpriteButton)) {
                    string savePath = "./sprites/costume_" + to_string(sprites.size()+1) + ".png";
                    SDL_Texture *costumeTex = launchCostumeEditor(renderer, savePath);

                    SpriteEntry new_sprite;
                    new_sprite.name = "Sprite" + to_string(sprites.size()+1);

                    if (costumeTex) {
                        new_sprite.sprite    = createSprite(costumeTex, savePath,
                                                    stage_starting_x*2 + satge_width,
                                                    (60)*2 + (window_screen_height-160));
                        new_sprite.thumbnail = costumeTex;
                        new_sprite.costumePaths.push_back(savePath);
                    } else {
                        SDL_Texture *placeholderTexture = SDL_CreateTexture(renderer,
                            SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 1, 1);
                        Uint32 color = 0xFF6464FF;
                        SDL_UpdateTexture(placeholderTexture, nullptr, &color, 4);
                        new_sprite.sprite    = createSprite(placeholderTexture, "default",
                                                    stage_starting_x*2 + satge_width,
                                                    (60)*2 + (window_screen_height-160));
                        new_sprite.thumbnail = placeholderTexture;
                        new_sprite.costumePaths.push_back("");
                    }
                    sprites.push_back(new_sprite);
                    state.activeSprite = (int)sprites.size() - 1;
                }

                {
                    int thumbnailWidth=70,thumbnailHeight=70,thumbnailPadding=8;
                    int startX=ui.addSpriteButton.x+ui.addSpriteButton.w+thumbnailPadding;
                    for (int i=0;i<(int)sprites.size();i++) {
                        SDL_Rect thumbnailRect={startX+i*(thumbnailWidth+thumbnailPadding),ui.spriteListPanel.y+8,thumbnailWidth,thumbnailHeight};
                        if (SDL_PointInRect(&mousePoint,&thumbnailRect)) state.activeSprite=i;
                    }
                }

                if (SDL_PointInRect(&mousePoint,&ui.scriptCanvas) && state.placeTab==Scripts_Tab) {
                    bool consumed=false;

                    for (int i=(int)scriptBlocks.size()-1;i>=0&&!consumed;i--) {
                        auto &iterBlock=scriptBlocks[i];
                        for (int j=0;j<(int)iterBlock.input_Rects.size()&&!consumed;j++) {
                            if (SDL_PointInRect(&mousePoint,&iterBlock.input_Rects[j])) {
                                if (state.editBlockIdx>=0 && state.editBlockIdx!=i)
                                    scriptBlocks[state.editBlockIdx].active_Input=-1;
                                iterBlock.active_Input=j;
                                state.editBlockIdx=i;
                                state.editInputIdx=j;
                                consumed=true;
                            }
                        }
                    }

                    if (!consumed) {
                        if (state.editBlockIdx>=0)
                            scriptBlocks[state.editBlockIdx].active_Input=-1;
                        state.editBlockIdx=state.editInputIdx=-1;

                        for (int i=(int)scriptBlocks.size()-1;i>=0&&!consumed;i--) {
                            SDL_Rect blockRect={scriptBlocks[i].x,scriptBlocks[i].y,220,block_height};
                            if (SDL_PointInRect(&mousePoint,&blockRect)) {
                                state.dragScriptIdx=i;
                                state.dragScriptOffX=mousePoint.x-scriptBlocks[i].x;
                                state.dragScriptOffY=mousePoint.y-scriptBlocks[i].y;
                                scriptBlocks[i].snapParent=-1;
                                consumed=true;
                            }
                        }
                    }
                }

                if (SDL_PointInRect(&mousePoint,&ui.Placing_BlockArea) && state.placeTab==Scripts_Tab) {
                    auto &cat=Placing_ette[state.Placing_Tab];
                    for (int i=0;i<(int)cat.size();i++) {
                        SDL_Rect paletteBlockRect={ui.Placing_BlockArea.x+8,
                                     ui.Placing_BlockArea.y+8+i*(block_height+blocks_gap)-state.Placing_ScrollY,
                                     ui.Placing_BlockArea.w-16,block_height};
                        if (SDL_PointInRect(&mousePoint,&paletteBlockRect)) {
                            state.dragSrcIdx=i;
                            state.dragOffX=mousePoint.x-paletteBlockRect.x;
                            state.dragOffY=mousePoint.y-paletteBlockRect.y;
                        }
                    }
                }

                if (!sprites.empty()) spriteDrag(sprites[state.activeSprite].sprite,sdlEvent);
            }

            if (sdlEvent.type==SDL_MOUSEBUTTONDOWN && sdlEvent.button.button==SDL_BUTTON_RIGHT) {
                SDL_Point mousePoint={sdlEvent.button.x,sdlEvent.button.y};

                state.contextMenuOpen = false;
                state.costumeCtx_Open  = false;

                int thumbnailWidth=70,thumbnailHeight=70,thumbnailPadding=8;
                int startX=ui.addSpriteButton.x+ui.addSpriteButton.w+thumbnailPadding;
                for (int i=0;i<(int)sprites.size();i++) {
                    SDL_Rect thumbnailRect={startX+i*(thumbnailWidth+thumbnailPadding),ui.spriteListPanel.y+8,thumbnailWidth,thumbnailHeight};
                    if (SDL_PointInRect(&mousePoint,&thumbnailRect)) {
                        state.contextSpriteIdx = i;
                        state.contextMenuX     = sdlEvent.button.x;
                        state.contextMenuY     = sdlEvent.button.y;
                        state.contextMenuOpen  = true;
                    }
                }

                if (state.placeTab == Customs_Tab && !sprites.empty()) {
                    auto &new_sprite = sprites[state.activeSprite];
                    int costumeAreaX = ui.scriptCanvas.x + 10;
                    int costumeAreaY = ui.scriptCanvas.y + 60;
                    for (int i = 0; i < (int)new_sprite.sprite.spriteCostumes.size(); i++) {
                        SDL_Rect frame = {costumeAreaX, costumeAreaY + i*90, 80, 80};
                        if (SDL_PointInRect(&mousePoint, &frame)) {
                            state.selected_costume_id = i;
                            state.costumeCtx_X   = sdlEvent.button.x;
                            state.costumeCtx_Y   = sdlEvent.button.y;
                            state.costumeCtx_Open = true;
                        }
                    }
                }
            }

            if (sdlEvent.type==SDL_MOUSEBUTTONUP && sdlEvent.button.button==SDL_BUTTON_LEFT) {
                SDL_Point mousePoint={sdlEvent.button.x,sdlEvent.button.y};

                if (state.dragSrcIdx>=0) {
                    if (SDL_PointInRect(&mousePoint,&ui.scriptCanvas) && state.placeTab==Scripts_Tab) {
                        ScriptBlock droppedBlock;
                        droppedBlock.placed_block_in_canvas=Placing_ette[state.Placing_Tab][state.dragSrcIdx];
                        droppedBlock.x=mousePoint.x-state.dragOffX;
                        droppedBlock.y=mousePoint.y-state.dragOffY;
                        droppedBlock.input_Values.resize(countInputs(droppedBlock.placed_block_in_canvas.label));

                        auto sr = findSnapTarget(scriptBlocks,-1,droppedBlock.x,droppedBlock.y);
                        if (sr.targetIdx >= 0) {
                            auto &tgt = scriptBlocks[sr.targetIdx];
                            droppedBlock.x = tgt.x;
                            droppedBlock.y = tgt.y + blockTotalHeight(scriptBlocks, sr.targetIdx);
                            droppedBlock.snapParent = sr.targetIdx;

                            droppedBlock.container_Parent = tgt.container_Parent;
                            droppedBlock.container_Branch = tgt.container_Branch;
                        } else if (sr.intoContainer) {
                            droppedBlock.x = scriptBlocks[sr.containerIdx].x + CONTAINER_INDENT;
                            droppedBlock.y = containerInnerY(scriptBlocks, sr.containerIdx, sr.containerBranch);
                            droppedBlock.container_Parent = sr.containerIdx;
                            droppedBlock.container_Branch = sr.containerBranch;
                        }
                        scriptBlocks.push_back(move(droppedBlock));

                        if (scriptBlocks.back().container_Parent >= 0)
                            propagateContainerChildren(scriptBlocks, scriptBlocks.back().container_Parent);
                    }
                    state.dragSrcIdx=-1;
                }

                if (state.dragScriptIdx>=0) {
                    auto &draggedBlock=scriptBlocks[state.dragScriptIdx];
                    if (!SDL_PointInRect(&mousePoint,&ui.scriptCanvas)) {

                        scriptBlocks.erase(scriptBlocks.begin()+state.dragScriptIdx);
                        for (auto &b:scriptBlocks) {
                            if (b.snapParent == state.dragScriptIdx) b.snapParent = -1;
                            else if (b.snapParent > state.dragScriptIdx) b.snapParent--;
                            if (b.container_Parent == state.dragScriptIdx) b.container_Parent = -1;
                            else if (b.container_Parent > state.dragScriptIdx) b.container_Parent--;
                        }
                    } else {

                        draggedBlock.snapParent = -1;
                        int oldContainer = draggedBlock.container_Parent;
                        draggedBlock.container_Parent = -1;

                        auto sr = findSnapTarget(scriptBlocks, state.dragScriptIdx, draggedBlock.x, draggedBlock.y);
                        if (sr.targetIdx >= 0) {
                            auto &tgt = scriptBlocks[sr.targetIdx];
                            draggedBlock.x = tgt.x;
                            draggedBlock.y = tgt.y + blockTotalHeight(scriptBlocks, sr.targetIdx);
                            draggedBlock.snapParent = sr.targetIdx;
                            draggedBlock.container_Parent = tgt.container_Parent;
                            draggedBlock.container_Branch = tgt.container_Branch;
                        } else if (sr.intoContainer) {
                            draggedBlock.x = scriptBlocks[sr.containerIdx].x + CONTAINER_INDENT;
                            draggedBlock.y = containerInnerY(scriptBlocks, sr.containerIdx, sr.containerBranch);
                            draggedBlock.container_Parent = sr.containerIdx;
                            draggedBlock.container_Branch = sr.containerBranch;
                        }
                        propagateSnap(scriptBlocks, state.dragScriptIdx);

                        if (oldContainer >= 0) propagateContainerChildren(scriptBlocks, oldContainer);
                        if (draggedBlock.container_Parent >= 0) propagateContainerChildren(scriptBlocks, draggedBlock.container_Parent);
                    }
                    state.dragScriptIdx=-1;
                }

                if (!sprites.empty()) spriteDrag(sprites[state.activeSprite].sprite,sdlEvent);
            }
        }

        if (state.greenFlagOn) {
            auto nonBlockingStep = [](Interpreter &interp) {
                if (interp.execStack.empty()) { interp.isRunning = false; return; }
                ExecutionFrame &top = interp.execStack.back();
                Block *blk = top.currentBlock;
                switch (blk->type) {
                    case BlockType::Forever: {
                        if (blk->children.empty()) { interp.execStack.pop_back(); return; }
                        if (top.childIndex >= (int)blk->children.size()) top.childIndex = 0;
                        Block *child = blk->children[top.childIndex].get();
                        if (child->type == BlockType::StopAll) {
                            interp.isRunning = false; interp.execStack.pop_back();
                        } else { checkAndRun(child); top.childIndex++; }
                        return;
                    }
                    case BlockType::Repeat: {
                        if (top.childIndex == 0 && blk->remainingIterations == 0)
                            blk->remainingIterations = static_cast<int>(blk->parameters.at(0).numVal);
                        if (blk->remainingIterations <= 0) { interp.execStack.pop_back(); return; }
                        if (top.childIndex >= (int)blk->children.size()) {
                            blk->remainingIterations--;
                            top.childIndex = 0;
                            if (blk->remainingIterations <= 0) interp.execStack.pop_back();
                        } else { checkAndRun(blk->children[top.childIndex].get()); top.childIndex++; }
                        return;
                    }
                    case BlockType::RepeatUntil: {
                        if (blk->parameters.at(0).boolVal) { interp.execStack.pop_back(); return; }
                        if (top.childIndex >= (int)blk->children.size()) top.childIndex = 0;
                        else { checkAndRun(blk->children[top.childIndex].get()); top.childIndex++; }
                        return;
                    }
                    case BlockType::If_Then: {
                        if (!blk->parameters.at(0).boolVal || top.childIndex >= (int)blk->children.size()) {
                            interp.execStack.pop_back(); return;
                        }
                        checkAndRun(blk->children[top.childIndex].get());
                        top.childIndex++;
                        if (top.childIndex >= (int)blk->children.size()) interp.execStack.pop_back();
                        return;
                    }
                    case BlockType::If_Then_Else: {
                        auto &branch = blk->parameters.at(0).boolVal ? blk->children : blk->second_children;
                        if (top.childIndex >= (int)branch.size()) { interp.execStack.pop_back(); return; }
                        checkAndRun(branch[top.childIndex].get());
                        top.childIndex++;
                        if (top.childIndex >= (int)branch.size()) interp.execStack.pop_back();
                        return;
                    }
                    case BlockType::WaitUntil: {
                        if (blk->parameters.at(0).boolVal) interp.execStack.pop_back();
                        return;
                    }
                    case BlockType::StopAll: {
                        interp.isRunning = false; interp.execStack.pop_back(); return;
                    }
                    case BlockType::Wait: {
                        Uint32 ms = static_cast<Uint32>(blk->parameters.at(0).numVal);
                        if (blk->remainingIterations == 0)
                            blk->remainingIterations = (int)(SDL_GetTicks() + ms);
                        if (SDL_GetTicks() >= (Uint32)blk->remainingIterations) {
                            blk->remainingIterations = 0; interp.execStack.pop_back();
                        }
                        return;
                    }
                    default:
                        if (blk->type == BlockType::IfOnEdgeBounce
                            && !blk->parameters.empty() && blk->parameters[0].sprPtr) {
                            Sprite &spr = *blk->parameters[0].sprPtr;
                            double halfWidth = spr.costumeWidth  * spr.spriteSize / 200.0;
                            double halfHeight = spr.costumeHeight * spr.spriteSize / 200.0;
                            bool hitLR = (spr.xCenter - halfWidth < gameArea.x || spr.xCenter + halfWidth > gameArea.x + gameArea.w);
                            bool hitTB = (spr.yCenter - halfHeight < gameArea.y || spr.yCenter + halfHeight > gameArea.y + gameArea.h);
                            if (hitLR) {
                                spr.direction = 180.0 - spr.direction;
                                if (spr.xCenter - halfWidth < gameArea.x) spr.xCenter = gameArea.x + halfWidth;
                                if (spr.xCenter + halfWidth > gameArea.x + gameArea.w) spr.xCenter = gameArea.x + gameArea.w - halfWidth;
                            }
                            if (hitTB) {
                                spr.direction = -spr.direction;
                                if (spr.yCenter - halfHeight < gameArea.y) spr.yCenter = gameArea.y + halfHeight;
                                if (spr.yCenter + halfHeight > gameArea.y + gameArea.h) spr.yCenter = gameArea.y + gameArea.h - halfHeight;
                            }
                        } else { checkAndRun(blk); }
                        interp.execStack.pop_back(); return;
                }
            };
            for (auto *interp : multiInterpreter.interpreters)
                if (interp->isRunning) nonBlockingStep(*interp);
        }

        SDL_SetRenderDrawColor(renderer,30,30,30,255);
        SDL_RenderClear(renderer);

        {
            SDL_SetRenderDrawColor(renderer,45,45,55,255);
            SDL_RenderFillRect(renderer,&ui.Placing_ettePanel);

            for (int i=0;i<Placing_Tab_COUNT;i++) {
                SDL_Color tabColor=placing_tab_colors[i];
                if (i!=state.Placing_Tab){tabColor.r/=2;tabColor.g/=2;tabColor.b/=2;}
                drawRect(renderer,ui.Placing_TabButtons[i],tabColor);
                renderTextCentered(renderer,FontSmall,placing_tabs_names[i],ui.Placing_TabButtons[i]);
                SDL_SetRenderDrawColor(renderer,20,20,20,255);
                SDL_Rect sep={ui.Placing_TabButtons[i].x,ui.Placing_TabButtons[i].y+ui.Placing_TabButtons[i].h-1,ui.Placing_TabButtons[i].w,1};
                SDL_RenderFillRect(renderer,&sep);
            }

            SDL_RenderSetClipRect(renderer,&ui.Placing_BlockArea);
            auto &cat=Placing_ette[state.Placing_Tab];
            SDL_Color paletteBlockColor=placing_tab_colors[state.Placing_Tab];
            for (int i=0;i<(int)cat.size();i++) {
                SDL_Rect paletteBlockRect={ui.Placing_BlockArea.x+8,
                             ui.Placing_BlockArea.y+8+i*(block_height+blocks_gap)-state.Placing_ScrollY,
                             ui.Placing_BlockArea.w-16,block_height};
                if (paletteBlockRect.y+paletteBlockRect.h<ui.Placing_BlockArea.y) continue;
                if (paletteBlockRect.y>ui.Placing_BlockArea.y+ui.Placing_BlockArea.h) break;
                drawRect(renderer,paletteBlockRect,paletteBlockColor);
                renderText(renderer,FontSmall,cat[i].label,paletteBlockRect.x+6,paletteBlockRect.y+(block_height-13)/2);
            }
            SDL_RenderSetClipRect(renderer,nullptr);
        }

        {
            SDL_SetRenderDrawColor(renderer,55,55,65,255);
            SDL_RenderFillRect(renderer,&ui.middlePanel);

            auto tabButton=[&](SDL_Rect Button,const char *lbl,int t){
                drawRect(renderer,Button,state.placeTab==t?SDL_Color{80,120,200,255}:SDL_Color{60,60,70,255});
                renderTextCentered(renderer,FontSmall,lbl,Button);
            };
            tabButton(ui.scriptButton,"Scripts",Scripts_Tab);
            tabButton(ui.costumeButton,"Costumes",Customs_Tab);
            tabButton(ui.soundButton,"Sounds",Sounds_Tab);

            SDL_SetRenderDrawColor(renderer,40,40,50,255);
            SDL_RenderFillRect(renderer,&ui.scriptCanvas);

            if (state.placeTab==Scripts_Tab) {
                SDL_RenderSetClipRect(renderer,&ui.scriptCanvas);

                auto drawSnapGuide=[&](int movingIdx, int movingx, int movingy){
                    auto sr = findSnapTarget(scriptBlocks,movingIdx,movingx,movingy);
                    SDL_SetRenderDrawColor(renderer,255,240,80,220);
                    if (sr.targetIdx >= 0) {
                        int lineY = scriptBlocks[sr.targetIdx].y
                                  + blockTotalHeight(scriptBlocks, sr.targetIdx);
                        SDL_RenderDrawLine(renderer,
                            scriptBlocks[sr.targetIdx].x, lineY,
                            scriptBlocks[sr.targetIdx].x+220, lineY);
                    } else if (sr.intoContainer) {
                        auto &c = scriptBlocks[sr.containerIdx];
                        int lineY = containerInnerY(scriptBlocks, sr.containerIdx, sr.containerBranch);
                        SDL_RenderDrawLine(renderer,
                            c.x + CONTAINER_INDENT, lineY,
                            c.x + CONTAINER_INDENT + 200, lineY);
                    }
                };
                if (state.dragScriptIdx>=0)
                    drawSnapGuide(state.dragScriptIdx,
                                  scriptBlocks[state.dragScriptIdx].x,
                                  scriptBlocks[state.dragScriptIdx].y);
                if (state.dragSrcIdx>=0)
                    drawSnapGuide(-1, state.mouseX-state.dragOffX,
                                      state.mouseY-state.dragOffY);

                for (int i=0;i<(int)scriptBlocks.size();i++)
                    renderScriptBlock(renderer,scriptBlocks[i],scriptBlocks,i);

                SDL_RenderSetClipRect(renderer,nullptr);

                if (scriptBlocks.empty())
                    renderTextCentered(renderer,FontSmall,
                        "Drag blocks here from the Placing_ette",
                        ui.scriptCanvas,{85,85,100,255});

                renderText(renderer,FontSmall,
                    "Click a white box to edit its value  |  Drag block outside to delete",
                    ui.scriptCanvas.x+8,
                    ui.scriptCanvas.y+ui.scriptCanvas.h-18,
                    {70,70,85,255});

            } else if (state.placeTab==Customs_Tab) {

                SDL_Rect changeCosButton = {ui.scriptCanvas.x+10, ui.scriptCanvas.y+10, 160, 34};
                drawRect(renderer, changeCosButton, {70,130,200,255});
                renderTextCentered(renderer, FontSmall, "+ Change Costume", changeCosButton);

                if (!sprites.empty()) {
                    auto &new_sprite = sprites[state.activeSprite];
                    int costumeStartX = ui.scriptCanvas.x + 10;
                    int costumeStartY = ui.scriptCanvas.y + 60;
                    for (int i = 0; i < (int)new_sprite.sprite.spriteCostumes.size(); i++) {
                        SDL_Rect frame = {costumeStartX, costumeStartY + i*90, 80, 80};
                        SDL_SetRenderDrawColor(renderer,
                            i==new_sprite.sprite.currentCostumeIndex ? 100 : 60,
                            i==new_sprite.sprite.currentCostumeIndex ? 160 : 60,
                            i==new_sprite.sprite.currentCostumeIndex ? 240 : 70, 255);
                        SDL_RenderFillRect(renderer, &frame);
                        if (new_sprite.sprite.spriteCostumes[i].costumeTexture)
                            SDL_RenderCopy(renderer,
                                new_sprite.sprite.spriteCostumes[i].costumeTexture,
                                nullptr, &frame);
                        renderText(renderer, FontSmall,
                            "Costume " + to_string(i+1),
                            costumeStartX+86, costumeStartY + i*90 + 32, {200,200,200,255});
                        if (i == new_sprite.sprite.currentCostumeIndex)
                            renderText(renderer, FontSmall, "(active)",
                                costumeStartX+86, costumeStartY + i*90 + 50, {120,200,120,255});
                    }
                } else {
                    renderTextCentered(renderer, FontSmall,
                        "Add a sprite first", ui.scriptCanvas, {85,85,100,255});
                }

            } else {

                SDL_Rect addSoundButton = {ui.scriptCanvas.x+10, ui.scriptCanvas.y+10, 140, 34};
                drawRect(renderer, addSoundButton, {220,105,132,255});
                renderTextCentered(renderer, FontSmall, "+ Add Sound", addSoundButton);

                if (!sprites.empty()) {
                    auto &new_sprite = sprites[state.activeSprite];
                    if (new_sprite.soundNames.empty()) {
                        renderText(renderer, FontSmall,
                            "No sounds yet. Click \"+ Add Sound\" to load a WAV / MP3.",
                            ui.scriptCanvas.x+10, ui.scriptCanvas.y+60,
                            {120,120,140,255});
                    } else {
                        renderText(renderer, FontSmall,
                            "Sounds for " + new_sprite.name + ":",
                            ui.scriptCanvas.x+10, ui.scriptCanvas.y+56,
                            {200,200,200,255});
                        for (int i=0; i<(int)new_sprite.soundNames.size(); i++) {
                            SDL_Rect row = {ui.scriptCanvas.x+10,
                                           ui.scriptCanvas.y+76+i*38,
                                           ui.scriptCanvas.w-20, 32};
                            SDL_SetRenderDrawColor(renderer,65,50,75,255);
                            SDL_RenderFillRect(renderer,&row);
                            renderText(renderer, FontSmall,
                                "♪  " + new_sprite.soundNames[i],
                                row.x+8, row.y+8, {230,180,220,255});
                        }
                    }
                } else {
                    renderTextCentered(renderer, FontSmall,
                        "Add a sprite first", ui.scriptCanvas, {85,85,100,255});
                }
            }
        }

        {
            SDL_SetRenderDrawColor(renderer,35,35,40,255);
            SDL_RenderFillRect(renderer,&ui.stagePanel);

            drawRect(renderer,ui.greenFlagButton,
                state.greenFlagOn?SDL_Color{50,200,80,255}:SDL_Color{30,140,50,255});
            renderTextCentered(renderer,FontLarge,">",ui.greenFlagButton);

            drawRect(renderer,ui.stopButton,{180,50,50,255});
            renderTextCentered(renderer,FontLarge,"■",ui.stopButton);

            drawRect(renderer,ui.saveButton,{60,130,180,255});
            renderTextCentered(renderer,FontSmall,"💾 Save",ui.saveButton);

            drawRect(renderer,ui.loadButton,{80,160,100,255});
            renderTextCentered(renderer,FontSmall,"📂 Load",ui.loadButton);

            if (!state.statusMsg.empty()) {
                if (SDL_GetTicks() - state.statusMsgTime < 3000)
                    renderText(renderer, FontSmall, state.statusMsg, ui.saveButton.x, ui.saveButton.y+48, {180,230,180,255});
                else state.statusMsg.clear();
            }

            SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderFillRect(renderer,&ui.stageView);
            drawStage(renderer,stage);
            for (auto &new_sprite:sprites) drawSprite(renderer,new_sprite.sprite,SayBubbleTex,ThinkBubbleTex);
            SDL_SetRenderDrawColor(renderer,80,80,90,255);
            SDL_RenderDrawRect(renderer,&ui.stageView);

            SDL_SetRenderDrawColor(renderer,45,45,55,255);
            SDL_RenderFillRect(renderer,&ui.spriteListPanel);
            drawRect(renderer,ui.addSpriteButton,{70,130,200,255});
            renderTextCentered(renderer,FontSmall,"+ Sprite",ui.addSpriteButton);
            int thumbnailWidth=70,thumbnailHeight=70,thumbnailPadding=8;
            int startX=ui.addSpriteButton.x+ui.addSpriteButton.w+thumbnailPadding;
            for (int i=0;i<(int)sprites.size();i++) {
                SDL_Rect thumbnailRect={startX+i*(thumbnailWidth+thumbnailPadding),ui.spriteListPanel.y+8,thumbnailWidth,thumbnailHeight};
                if (i==state.activeSprite){
                    SDL_SetRenderDrawColor(renderer,100,160,240,255);
                    SDL_Rect hl={thumbnailRect.x-2,thumbnailRect.y-2,thumbnailRect.w+4,thumbnailRect.h+4};
                    SDL_RenderFillRect(renderer,&hl);
                }
                SDL_SetRenderDrawColor(renderer,60,60,70,255);
                SDL_RenderFillRect(renderer,&thumbnailRect);
                if (sprites[i].thumbnail) SDL_RenderCopy(renderer,sprites[i].thumbnail,nullptr,&thumbnailRect);
                renderText(renderer,FontSmall,sprites[i].name,thumbnailRect.x+2,thumbnailRect.y+thumbnailRect.h+2,{200,200,200,255});
            }
        }

        if (state.dragSrcIdx>=0) {
            auto &cat=Placing_ette[state.Placing_Tab];
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
        SDL_Rect ld={shop_screen_width-2,0,2,window_screen_height};     SDL_RenderFillRect(renderer,&ld);
        SDL_Rect md={shop_screen_width+placing_screen_width-2,0,2,window_screen_height}; SDL_RenderFillRect(renderer,&md);

        auto drawContextMenu = [&](int x, int y,
                                   const char *items[], SDL_Color itemBg[],
                                   int count) {
            int menuW = 180, itemH = 28;
            for (int i = 0; i < count; i++) {
                SDL_Rect row    = {x, y + i*itemH, menuW, itemH};
                SDL_Rect shadow = {row.x+2, row.y+2, row.w, row.h};
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer,0,0,0,120);
                SDL_RenderFillRect(renderer, &shadow);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                drawRect(renderer, row, itemBg[i]);
                SDL_SetRenderDrawColor(renderer,200,200,200,255);
                SDL_RenderDrawRect(renderer,&row);
                renderTextCentered(renderer, FontSmall, items[i], row);
            }
        };

        if (state.contextMenuOpen && state.contextSpriteIdx >= 0
            && state.contextSpriteIdx < (int)sprites.size()) {
            const char *items[] = { "Edit Costume", "Delete Sprite" };
            SDL_Color   cols[]  = { {60,100,180,255}, {180,60,60,255} };
            drawContextMenu(state.contextMenuX, state.contextMenuY, items, cols, 2);
        }

        if (state.costumeCtx_Open && !sprites.empty()
            && state.selected_costume_id >= 0
            && state.selected_costume_id < (int)sprites[state.activeSprite].sprite.spriteCostumes.size()) {
            bool canDelete = (int)sprites[state.activeSprite].sprite.spriteCostumes.size() > 1;
            const char *items[] = { "Edit Costume", canDelete ? "Delete Costume" : "(can't delete last)" };
            SDL_Color   cols[]  = { {60,100,180,255}, canDelete ? SDL_Color{180,60,60,255} : SDL_Color{80,80,80,255} };
            drawContextMenu(state.costumeCtx_X, state.costumeCtx_Y, items, cols, 2);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput();
    for (auto &new_sprite:sprites) freeAllSounds(new_sprite.sprite);
    for (auto *b:codeSpace) delete b;
    codeSpace.clear();
    for (auto *interp : multiInterpreter.interpreters) delete interp;
    multiInterpreter.interpreters.clear();
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