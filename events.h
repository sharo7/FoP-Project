#ifndef SCRATCH_EVENTS_H
#define SCRATCH_EVENTS_H

#include <bits/stdc++.h>

using namespace std;

struct Variable;

bool whenGreenFlagClicked(bool isClicked);

bool whenSomeKeyPressed(const string &key);

bool whenThisSpriteClicked(bool isClicked);

bool whenXisGreaterThanY(const Variable &X, double Y);

#endif //SCRATCH_EVENTS_H
