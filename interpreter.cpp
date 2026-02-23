#include "interpreter.h"

// this function is built to make the makeInterpreters cleaner.
void registerEvent(MultiInterpreter &m, Block *block, EventType type) {
    m.interpreters.emplace_back();
    m.events.emplace_back(Event{type});
    // Get the last interpreter created
    Interpreter &currentInterpreter = *multiInterpreter.interpreters.back();
    // add this block into it
    currentInterpreter.execStack.push_back(ExecutionFrame{block});
}

// Add a series of blocks (from currentBlock onwards) to the execStack of currentInterpreter
void findSeriesOfBlocks(const vector<Block *> &codeSpace, Interpreter &currentInterpreter, Block *currentBlock) {
    if (!currentBlock)
        return;

    // Quickly check if the starting block is in the codeSpace
    unordered_set<Block *> codeSet(codeSpace.begin(), codeSpace.end());
    if (codeSet.find(currentBlock) == codeSet.end())
        return; // Invalid start

    // To avoid loops
    unordered_set<Block *> visited;

    Block *b = currentBlock;
    while (b != nullptr) {
        // If we've already seen it => loop, break
        if (visited.find(b) != visited.end())
            break;
        visited.insert(b);

        // Add execution frame for this block
        currentInterpreter.execStack.push_back(ExecutionFrame{b, 0});

        // Check if the next block (connected to this block) is valid
        Block *next = b->isConnectedTo;
        if (next == nullptr)
            break; // End of chain
        if (codeSet.find(next) == codeSet.end())
            break; // Next block is outside project space => break

        // Here connection is valid, let's continue
        b = next;
    }
}

void makeInterpreters(const vector<Block *> &codeSpace, MultiInterpreter &multiInterpreter) {
    for (Block *block: codeSpace)
        switch (block->type) {
            case BlockType::WhenGreenFlagClicked: {
                registerEvent(multiInterpreter, block, WHEN_GREEN_FLAG_CLICKED);
                Interpreter &currentInterpreter = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter, block);
                break;
            }

            case BlockType::WhenSomeKeyPressed: {
                registerEvent(multiInterpreter, block, WHEN_SOME_KEY_PRESSED);
                Interpreter &currentInterpreter = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter, block);
                break;
            }

            case BlockType::WhenThisSpriteClicked: {
                registerEvent(multiInterpreter, block, WHEN_THIS_SPRITE_CLICKED);
                Interpreter &currentInterpreter = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter, block);
                break;
            }

            case BlockType::WhenXIsGreaterThanY: {
                registerEvent(multiInterpreter, block, WHEN_X_IS_GREATER_THAN_Y);
                Interpreter &currentInterpreter = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter, block);
                break;
            }

            default:
                break;
        }
}

/*
 * in main.cpp this will be written in this way:
 * makeInterpreters(codeSpace, multiInterpreter);
 */

void checkAndRun(Block *parent) {
    // SPR(p): returns a reference to the real sprite.
    // If sprPtr is set (IDE path), it points directly into the sprites vector — modifications
    // are immediately visible on screen. Falls back to sprVal for programmatic use.
    auto SPR = [](Value &v) -> Sprite& {
        return v.sprPtr ? *v.sprPtr : v.sprVal;
    };

    switch (parent->type) {
        // Run: motion_commands
        case BlockType::Move:
            move(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::TurnAnticlockwise:
            turnAnticlockwise(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::TurnClockwise:
            turnClockwise(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::PointInDirection:
            pointInDirection(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::GoToXY:
            goToXY(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal,
                   parent->parameters.at(2).numVal);
            break;

        case BlockType::ChangeXBy:
            changeXBy(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::ChangeYBy:
            changeYBy(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::GoToMousePointer:
            goToMousePointer(SPR(parent->parameters.at(0)));
            break;

        case BlockType::GoToRandomPosition:
            goToRandomPosition(SPR(parent->parameters.at(0)));
            break;

        case BlockType::IfOnEdgeBounce:
            ifOnEdgeBounce(SPR(parent->parameters.at(0)));
            break;

        // Run: looks_commands
        case BlockType::Show:
            show(SPR(parent->parameters.at(0)));
            break;

        case BlockType::Hide:
            hide(SPR(parent->parameters.at(0)));
            break;

        case BlockType::AddCostume:
            addCostume(SPR(parent->parameters.at(0)), parent->parameters.at(1).txtVal,
                       parent->parameters.at(2).strVal);
            break;

        case BlockType::NextCostume:
            nextCostume(SPR(parent->parameters.at(0)));
            break;

        case BlockType::SwitchCostumeTo:
            switchCostumeTo(SPR(parent->parameters.at(0)), parent->parameters.at(1).strVal);
            break;

        case BlockType::SetSizeTo:
            setSizeTo(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::ChangeSizeBy:
            changeSizeBy(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::ClearGraphicEffects:
            clearGraphicEffects(SPR(parent->parameters.at(0)));
            break;

        case BlockType::SetColorEffectTo:
            setColorEffectTo(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::ChangeColorEffectBy:
            changeColorEffectBy(SPR(parent->parameters.at(0)), parent->parameters.at(1).numVal);
            break;

        case BlockType::AddBackdrop:
            addBackdrop(parent->parameters.at(0).stgVal, parent->parameters.at(1).txtVal,
                        parent->parameters.at(2).strVal);
            break;

        case BlockType::SwitchBackdropTo:
            switchBackdropTo(parent->parameters.at(0).stgVal, parent->parameters.at(1).strVal);
            break;

        case BlockType::NextBackdrop:
            nextBackdrop(parent->parameters.at(0).stgVal);
            break;

        case BlockType::CostumeNumber:
            costumeNumber(SPR(parent->parameters.at(0)));
            break;

        case BlockType::CostumeName:
            costumeName(SPR(parent->parameters.at(0)));
            break;

        case BlockType::BackdropNumber:
            backdropNumber(parent->parameters.at(0).stgVal);
            break;

        case BlockType::BackdropName:
            backdropName(parent->parameters.at(0).stgVal);
            break;

        case BlockType::Size:
            size(SPR(parent->parameters.at(0)));
            break;

        case BlockType::Say:
            say(SPR(parent->parameters.at(0)), parent->parameters.at(1).strVal,
                parent->parameters.at(2).rndVal, parent->parameters.at(3).fntVal);
            break;

        case BlockType::Think:
            think(SPR(parent->parameters.at(0)), parent->parameters.at(1).strVal,
                  parent->parameters.at(2).rndVal, parent->parameters.at(3).fntVal);
            break;

        case BlockType::SayForSeconds:
            sayForSeconds(SPR(parent->parameters.at(0)), parent->parameters.at(1).strVal,
                          parent->parameters.at(2).rndVal, parent->parameters.at(3).fntVal,
                          parent->parameters.at(4).numVal);
            break;

        case BlockType::ThinkForSeconds:
            thinkForSeconds(SPR(parent->parameters.at(0)), parent->parameters.at(1).strVal,
                            parent->parameters.at(2).rndVal, parent->parameters.at(3).fntVal,
                            parent->parameters.at(4).numVal);
            break;

        // Run: sound_commands
        case BlockType::AddSound:
            addSound(SPR(parent->parameters.at(0)),
                     parent->parameters.at(1).strVal,
                     parent->parameters.at(2).strVal);
            break;

        case BlockType::StartSound:
            startSound(SPR(parent->parameters.at(0)),
                       parent->parameters.at(1).strVal);
            break;

        case BlockType::PlaySoundUntilDone:
            playSoundUntilDone(SPR(parent->parameters.at(0)),
                               parent->parameters.at(1).strVal);
            break;

        case BlockType::StopAllSounds:
            stopAllSounds(SPR(parent->parameters.at(0)));
            break;

        case BlockType::SetVolumeTo:
            setVolumeTo(SPR(parent->parameters.at(0)),
                        parent->parameters.at(1).strVal,
                        static_cast<int>(parent->parameters.at(2).numVal));
            break;

        case BlockType::ChangeVolumeBy:
            changeVolumeBy(SPR(parent->parameters.at(0)),
                           parent->parameters.at(1).strVal,
                           static_cast<int>(parent->parameters.at(2).numVal));
            break;

        // Run: control_flow_logic
        case BlockType::Wait:
            wait(static_cast<int>(parent->parameters.at(0).numVal));
            break;

        // Run: sensing_commands
        case BlockType::DistanceToSprite:
            distanceToSprite(SPR(parent->parameters.at(0)), SPR(parent->parameters.at(1)));
            break;

        case BlockType::DistanceToMouse:
            distanceToMouse(SPR(parent->parameters.at(0)));
            break;

        case BlockType::Timer:
            timer();
            break;

        case BlockType::ResetTimer:
            resetTimer();
            break;

        case BlockType::TouchingMousePointer:
            touchingMousePointer(SPR(parent->parameters.at(0)));
            break;

        case BlockType::TouchingSprite:
            touchingSprite(SPR(parent->parameters.at(0)), SPR(parent->parameters.at(1)));
            break;

        case BlockType::TouchingEdge:
            touchingEdge(SPR(parent->parameters.at(0)));
            break;

        case BlockType::MouseX:
            mouseX();
            break;

        case BlockType::MouseY:
            mouseY();
            break;

        case BlockType::MouseDown:
            mouseDown();
            break;

        case BlockType::KeyPressed:
            keyPressed(parent->parameters.at(0).strVal);
            break;

        case BlockType::SetDragMode:
            setDragMode(SPR(parent->parameters.at(0)), parent->parameters.at(1).boolVal);
            break;

        // Run: operators
        case BlockType::Addition:
            addition(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::Subtraction:
            subtraction(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::Multiplication:
            multiplication(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::Division:
            division(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::Modulos:
            myModulus(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::IsEqual:
            isEqual(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::IsGreaterThan:
            isGreaterThan(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::IsLessThan:
            isLessThan(parent->parameters.at(0).numVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::MyAbs:
            myAbs(parent->parameters.at(0).numVal);
            break;

        case BlockType::MySqrt:
            mySqrt(parent->parameters.at(0).numVal);
            break;

        case BlockType::MyFloor:
            myFloor(parent->parameters.at(0).numVal);
            break;

        case BlockType::MyCeil:
            myCeil(parent->parameters.at(0).numVal);
            break;

        case BlockType::MySinus:
            mySinus(parent->parameters.at(0).numVal);
            break;

        case BlockType::MyCosine:
            myCosine(parent->parameters.at(0).numVal);
            break;

        case BlockType::MyAnd:
            myAnd(parent->parameters.at(0).boolVal, parent->parameters.at(1).boolVal);
            break;

        case BlockType::MyOr:
            myOr(parent->parameters.at(0).boolVal, parent->parameters.at(1).boolVal);
            break;

        case BlockType::MyNot:
            myNot(parent->parameters.at(0).boolVal);
            break;

        case BlockType::MyXor:
            myXor(parent->parameters.at(0).boolVal, parent->parameters.at(1).boolVal);
            break;

        case BlockType::LengthOfString:
            lengthOfString(parent->parameters.at(0).strVal);
            break;

        case BlockType::CharAt:
            charAt(static_cast<int>(parent->parameters.at(0).numVal),
                   parent->parameters.at(1).strVal);
            break;

        case BlockType::StringConcat:
            stringConcat(parent->parameters.at(0).strVal, parent->parameters.at(1).strVal);
            break;

        // Run: variables
        case BlockType::CreateVariable:
            createVariable(parent->parameters.at(0).strVal, parent->parameters.at(1).boolVal,
                           static_cast<int>(parent->parameters.at(2).numVal));
            break;

        case BlockType::SetVariableValue:
            setVariableValue(parent->parameters.at(0).strVal,
                             static_cast<int>(parent->parameters.at(1).numVal),
                             parent->parameters.at(2));
            break;

        case BlockType::ChangeVariableValue:
            changeVariableValue(parent->parameters.at(0).strVal,
                                static_cast<int>(parent->parameters.at(1).numVal),
                                parent->parameters.at(2));
            break;

        case BlockType::ShowVariable:
            showVariable(parent->parameters.at(0).strVal,
                         static_cast<int>(parent->parameters.at(1).numVal));
            break;

        case BlockType::HideVariable:
            hideVariable(parent->parameters.at(0).strVal,
                         static_cast<int>(parent->parameters.at(1).numVal));
            break;

        case BlockType::GetVariableValue: {
            if (processValue(variables[parent->parameters.at(0).strVal].value) == 1)
                getVariableValue<double>(parent->parameters.at(0).strVal,
                                         static_cast<int>(parent->parameters.at(1).numVal));
            else if (processValue(variables[parent->parameters.at(0).strVal].value) == 2)
                getVariableValue<string>(parent->parameters.at(0).strVal,
                                         static_cast<int>(parent->parameters.at(1).numVal));
            else if (processValue(variables[parent->parameters.at(0).strVal].value) == 3)
                getVariableValue<bool>(parent->parameters.at(0).strVal,
                                       static_cast<int>(parent->parameters.at(1).numVal));
            break;
        }

        default:
            break;
    }
}

void runStep(Interpreter &interpreter) {
    // If the stack is empty, meaning all instructions have been executed
    if (interpreter.execStack.empty()) {
        interpreter.isRunning = false;
        return;
    }

    //  Access the last level of the stack (where we are now)
    ExecutionFrame &topFrame = interpreter.execStack.back();
    Block *parent = topFrame.currentBlock;

    //Run
    switch (parent->type) {
        case BlockType::Repeat: {
            parent->remainingIterations = static_cast<int>(parent->parameters.at(0).numVal);
            while (parent->remainingIterations) {
                // if all children of this level have finished executing
                if (topFrame.childIndex >= parent->children.size()) {
                    // if there are still iterations left, go back to the beginning
                    if (parent->remainingIterations > 1) {
                        parent->remainingIterations--;
                        topFrame.childIndex = 0; // Reset index to repeat
                    }
                } else if (topFrame.childIndex < parent->children.size()) {
                    // Select the next child to execute
                    Block *nextBlock = parent->children[topFrame.childIndex].get();
                    checkAndRun(nextBlock);
                    topFrame.childIndex++;
                }
            }
            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        case BlockType::Forever: {
            while (true) {
                // if all children of this level have finished executing
                if (topFrame.childIndex >= parent->children.size())
                    // go back to the beginning
                    topFrame.childIndex = 0; // Reset index to repeat
                else if (topFrame.childIndex < parent->children.size()) {
                    // Select the next child to execute
                    Block *nextBlock = parent->children[topFrame.childIndex].get();
                    if (nextBlock->type == BlockType::StopAll)
                        break;
                    checkAndRun(nextBlock);
                    topFrame.childIndex++;
                }
            }
            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        case BlockType::If_Then: {
            if (parent->parameters.at(0).boolVal) {
                parent->remainingIterations = 1;
                while (parent->remainingIterations) {
                    // if all children of this level have finished executing
                    if (topFrame.childIndex >= parent->children.size())
                        parent->remainingIterations--;
                    else if (topFrame.childIndex < parent->children.size()) {
                        // Select the next child to execute
                        Block *nextBlock = parent->children[topFrame.childIndex].get();
                        checkAndRun(nextBlock);
                        topFrame.childIndex++;
                    }
                }
            }
            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        case BlockType::WaitUntil: {
            while (!parent->parameters.at(0).boolVal)
                this_thread::sleep_for(chrono::milliseconds(100));

            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        case BlockType::StopAll: {
            interpreter.isRunning = false;
            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        case BlockType::If_Then_Else: {
            if (parent->parameters.at(0).boolVal) {
                parent->remainingIterations = 1;
                while (parent->remainingIterations) {
                    // if all children of this level have finished executing
                    if (topFrame.childIndex >= parent->children.size())
                        parent->remainingIterations--;
                    else if (topFrame.childIndex < parent->children.size()) {
                        // Select the next child to execute
                        Block *nextBlock = parent->children[topFrame.childIndex].get();
                        checkAndRun(nextBlock);
                        topFrame.childIndex++;
                    }
                }
            } else {
                parent->remainingIterations = 1;
                while (parent->remainingIterations) {
                    // if all children of this level have finished executing
                    if (topFrame.childIndex >= parent->second_children.size())
                        parent->remainingIterations--;
                    else if (topFrame.childIndex < parent->second_children.size()) {
                        // Select the next child to execute
                        Block *nextBlock = parent->second_children[topFrame.childIndex].get();
                        checkAndRun(nextBlock);
                        topFrame.childIndex++;
                    }
                }
            }
            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        case BlockType::RepeatUntil: {
            bool condition = parent->parameters.at(0).boolVal;
            while (!condition) {
                // if all children of this level have finished executing
                if (topFrame.childIndex >= parent->children.size()) {
                    // if the condition is still true, go back to the beginning
                    if (!condition)
                        topFrame.childIndex = 0; // Reset index to repeat
                } else if (topFrame.childIndex < parent->children.size()) {
                    // Select the next child to execute
                    Block *nextBlock = parent->children[topFrame.childIndex].get();
                    checkAndRun(nextBlock);
                    topFrame.childIndex++;
                }
            }
            // if this block is finished, pop it off the stack
            interpreter.execStack.pop_back();
            return;
        }

        default:
            break;
    }

    // in other cases:
    checkAndRun(parent);
    interpreter.execStack.pop_back();
}

/*
 * in main.cpp this will be written in this way:
 *
 * for (int i = 0; i < multi_interpreter.Interpreter.size(); i++)
 *     while(multi_interpreter.interpreters.at(i).isRunning)
 *         runStep(interpreter);
 *
 */
