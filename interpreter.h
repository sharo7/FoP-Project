#ifndef SCRATCH_INTERPRETER_H
#define SCRATCH_INTERPRETER_H

#include <bits/stdc++.h>
#include "sprite.h"
#include "motion_commands.h"
#include "looks_commands.h"
#include "sound_commands.h"
#include "events.h"
#include "control_flow_logic.h"
#include "sensing_commands.h"
#include "operators.h"
#include "variables.h"

using namespace std;

// Dynamic Data Type Support
enum DataType { NUMBER, STRING, BOOLEAN, SPRITE, STAGE, SDL_TEXTURE, SDL_RENDERER, TTF_FONT };

struct Value {
    DataType type;
    Sprite sprVal;
    Sprite* sprPtr = nullptr;  // points to the real sprite in the sprites vector
    Stage stgVal;
    double numVal;
    string strVal;
    bool boolVal;
    SDL_Texture *txtVal;
    SDL_Renderer *rndVal;
    TTF_Font *fntVal;
};

struct Variable {
    string name;
    Value value;
    bool isGlobal = true;
    int ownerSpriteId = -1;
    //if the variable is global then it's ownerSpriteId is -1.
    //otherwise it will be the ID of the sprite which is selected right now.
    bool show = false;
    //it determines if the variable should be shown in a table at the top of the main program
};

// A function that takes an anonymous input and decides based on its type
inline map<string, Variable> variables; // first is Variable's name

enum EventType { WHEN_GREEN_FLAG_CLICKED, WHEN_SOME_KEY_PRESSED, WHEN_THIS_SPRITE_CLICKED, WHEN_X_IS_GREATER_THAN_Y };

struct Event {
    EventType type;
    bool shallWeStart = false;
};

// Block Types
enum struct BlockType {
    // Motion Commands
    Move, TurnAnticlockwise, TurnClockwise, PointInDirection, GoToXY, ChangeXBy, ChangeYBy, GoToMousePointer,
    GoToRandomPosition, IfOnEdgeBounce,

    // Looks Commands
    Show, Hide, AddCostume, NextCostume, SwitchCostumeTo, SetSizeTo, ChangeSizeBy, ClearGraphicEffects,
    SetColorEffectTo, ChangeColorEffectBy, AddBackdrop, SwitchBackdropTo, NextBackdrop, CostumeNumber, CostumeName,
    BackdropNumber, BackdropName, Size, Say, Think, SayForSeconds, ThinkForSeconds,

    // Sound Commands
    AddSound, StartSound, PlaySoundUntilDone, StopAllSounds, SetVolumeTo, ChangeVolumeBy,

    // Events
    WhenGreenFlagClicked, WhenSomeKeyPressed, WhenThisSpriteClicked, WhenXIsGreaterThanY,

    // Control Flow Logic
    Wait, Repeat, Forever, If_Then, WaitUntil, StopAll, If_Then_Else, RepeatUntil,

    // Sensing Commands
    DistanceToSprite, DistanceToMouse, Timer, ResetTimer, TouchingMousePointer, TouchingSprite, TouchingEdge, MouseX,
    MouseY, MouseDown, KeyPressed, SetDragMode,

    //Operators
    Addition, Subtraction, Multiplication, Division, Modulos, IsEqual, IsGreaterThan, IsLessThan, MyAbs, MySqrt,
    MyFloor, MyCeil, MySinus, MyCosine, MyAnd, MyOr, MyNot, MyXor, LengthOfString, CharAt, StringConcat,

    //Variables
    CreateVariable, SetVariableValue, ChangeVariableValue, ShowVariable, HideVariable, GetVariableValue
};

// Project Structure
struct Block {
private:
    // A common counter for all Blocks
    static inline std::atomic<int> global_id{0};

public:
    int id;

    // Main constructor — generating a unique ID
    Block() : id(global_id++) {
    }

    // Prevent copying (so that no two objects have the same id)
    Block(const Block &) = delete;

    Block &operator=(const Block &) = delete;

    // Allow moving (for working with containers)
    Block(Block &&) = default;

    Block &operator=(Block &&) = default;

    BlockType type{};

    // This version is better if ownership is with the same Block
    vector<unique_ptr<Block> > children; // Blocks inside this block (for Repeat, Forever, If_Then)
    vector<unique_ptr<Block> > second_children; // Blocks inside this block (only for if_then_else)
    vector<Value> parameters; // Block inputs

    Block *isConnectedTo = nullptr; // // The block that comes after this block and is attached to the bottom of it.

    // Auxiliary variable to manage loop iterations
    int remainingIterations = 0;
};

// every block which will be added to this space, will be added here.
inline vector<Block *> codeSpace;

struct ExecutionFrame {
    Block *currentBlock; // The block whose children we are executing
    int childIndex; // Indicates which child is currently executing
};

struct Interpreter {
    vector<ExecutionFrame> execStack; // Execution stack
    bool isRunning = false;
};

// whenever an event command is set up, an interpreter will be added to this.
// and they will be executed in order with a for loop.
struct MultiInterpreter {
    vector<Interpreter *> interpreters;
    vector<Event> events;
};

inline MultiInterpreter multiInterpreter;
// a general variable which will be used in some places, especially in main.cpp

void registerEvent(MultiInterpreter &m, Block *block, EventType type);

void findSeriesOfBlocks(const vector<Block *> &codeSpace, Interpreter &currentInterpreter, Block *currentBlock);

void makeInterpreters(const vector<Block *> &codeSpace, MultiInterpreter &multiInterpreter);

void runStep(Interpreter &interpreter);

void checkAndRun(Block *parent);

#endif //SCRATCH_INTERPRETER_H
