#include "interpreter.h"

// this fuction is built to make the makeIntepreters cleaner.
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
            case BlockType::WhenGreenFlagClicked:
                registerEvent(multiInterpreter, block, WHEN_GREEN_FLAG_CLICKED);
                Interpreter &currentInterpreter_1 = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter_1, block);
                break;

            case BlockType::WhenSomeKeyPressed:
                registerEvent(multiInterpreter, block, WHEN_SOME_KEY_PRESSED);
                Interpreter &currentInterpreter_2 = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter_2, block);
                break;

            case BlockType::WhenThisSpriteClicked:
                registerEvent(multiInterpreter, block, WHEN_THIS_SPRITE_CLICKED);
                Interpreter &currentInterpreter_3 = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter_3, block);
                break;

            case BlockType::WhenXIsGreaterThanY:
                registerEvent(multiInterpreter, block, WHEN_X_IS_GREATER_THAN_Y);
                Interpreter &currentInterpreter_4 = *multiInterpreter.interpreters.back();
                findSeriesOfBlocks(codeSpace, currentInterpreter_4, block);
                break;

            default:
                break;
        }
}

/*
 * in main.cpp this will be written in this way:
 * makeInterpreters(codeSpace, multiInterpreter);
 */

void checkAndRun(Block *parent) {
    switch (parent->type) {
        // Run: motion_commands
        case BlockType::Move:
            move(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::TurnAnticlockwise:
            turnAnticlockwise(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::TurnClockwise:
            turnClockwise(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::PointInDirection:
            pointInDirection(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::GoToXY:
            goToXY(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal,
                   parent->parameters.at(2).numVal);
            break;

        case BlockType::ChangeXBy:
            changeXBy(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::ChangeYBy:
            changeYBy(parent->parameters.at(0).sprVal, parent->parameters.at(1).numVal);
            break;

        case BlockType::GoToMousePointer:
            goToMousePointer(parent->parameters.at(0).sprVal,
                             static_cast<int>(parent->parameters.at(1).numVal),
                             static_cast<int>(parent->parameters.at(2).numVal));
            break;

        case BlockType::GoToRandomPosition:
            goToRandomPosition(parent->parameters.at(0).sprVal,
                               static_cast<int>(parent->parameters.at(1).numVal),
                               static_cast<int>(parent->parameters.at(2).numVal));
            break;

        case BlockType::IfOnEdgeBounce:
            ifOnEdgeBounce(parent->parameters.at(0).sprVal,
                           static_cast<int>(parent->parameters.at(1).numVal),
                           static_cast<int>(parent->parameters.at(2).numVal));
            break;

        // Run: looks_commands
        case BlockType::Show:
            show(parent->parameters.at(0).sprVal);
            break;

        case BlockType::Hide:
            hide(parent->parameters.at(0).sprVal);
            break;

        // Run: sound_commands
        case BlockType::AddSound:
            addSound(parent->parameters.at(0).sprVal,
                     parent->parameters.at(1).strVal,
                     parent->parameters.at(2).strVal);
            break;

        case BlockType::StartSound:
            startSound(parent->parameters.at(0).sprVal,
                       parent->parameters.at(1).strVal);
            break;

        case BlockType::PlaySoundUntilDone:
            playSoundUntilDone(parent->parameters.at(0).sprVal,
                               parent->parameters.at(1).strVal);
            break;

        case BlockType::StopAllSounds:
            stopAllSounds(parent->parameters.at(0).sprVal);
            break;

        case BlockType::SetVolumeTo:
            setVolumeTo(parent->parameters.at(0).sprVal,
                        parent->parameters.at(1).strVal,
                        static_cast<int>(parent->parameters.at(2).numVal));
            break;

        case BlockType::ChangeVolumeBy:
            changeVolumeBy(parent->parameters.at(0).sprVal,
                           parent->parameters.at(1).strVal,
                           static_cast<int>(parent->parameters.at(2).numVal));
            break;

        // Run: control_flow_logic
        case BlockType::Wait:
            wait(static_cast<int>(parent->parameters.at(0).numVal));
            break;

        case BlockType::Repeat:

        // Run: operators
        case BlockType::Addition:
            addition(parent->parameters.at(0).numVal,
                     parent->parameters.at(1).numVal);
            break;

        case BlockType::Subtraction:
            subtraction(parent->parameters.at(0).numVal,
                        parent->parameters.at(1).numVal);
            break;

        case BlockType::Multiplication:
            multiplication(parent->parameters.at(0).numVal,
                           parent->parameters.at(1).numVal);
            break;

        case BlockType::Division:
            division(parent->parameters.at(0).numVal,
                     parent->parameters.at(1).numVal);
            break;

        case BlockType::Modulos:
            myModulus(parent->parameters.at(0).numVal,
                      parent->parameters.at(1).numVal);
            break;

        case BlockType::IsEqual:
            isEqual(parent->parameters.at(0).numVal,
                    parent->parameters.at(1).numVal);
            break;

        case BlockType::IsGreaterThan:
            isGreaterThan(parent->parameters.at(0).numVal,
                          parent->parameters.at(1).numVal);
            break;

        case BlockType::IsLessThan:
            isLessThan(parent->parameters.at(0).numVal,
                       parent->parameters.at(1).numVal);
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
            myAnd(parent->parameters.at(0).boolVal,
                  parent->parameters.at(1).boolVal);
            break;

        case BlockType::MyOr:
            myOr(parent->parameters.at(0).boolVal,
                 parent->parameters.at(1).boolVal);
            break;

        case BlockType::MyNot:
            myNot(parent->parameters.at(0).boolVal);
            break;

        case BlockType::MyXor:
            myXor(parent->parameters.at(0).boolVal,
                  parent->parameters.at(1).boolVal);
            break;

        case BlockType::LengthOfString:
            lengthOfString(parent->parameters.at(0).strVal);
            break;

        case BlockType::CharAt:
            charAt(static_cast<int>(parent->parameters.at(0).numVal),
                   parent->parameters.at(1).strVal);
            break;

        case BlockType::StringConcat:
            stringConcat(parent->parameters.at(0).strVal,
                         parent->parameters.at(1).strVal);
            break;

        // Run: variables
        case BlockType::CreateVariable:
            createVariable(parent->parameters.at(0).strVal, parent->parameters.at(1).boolVal,
                           static_cast<int>(parent->parameters.at(2).numVal));
            break;

        case BlockType::SetVariableValue:
            setVariableValue(parent->parameters.at(0).strVal,
                             static_cast<int>(parent->parameters.at(1).numVal),
                             parent->parameters.at(2).numVal);
            break;

        case BlockType::ChangeVariableValue:
            changeVariableValue(parent->parameters.at(0).strVal,
                                static_cast<int>(parent->parameters.at(1).numVal),
                                parent->parameters.at(2).numVal);
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
