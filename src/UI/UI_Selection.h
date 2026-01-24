#pragma once
#include "UI.h"
#include "../Area/Props.h"

extern uint32_t gSelectedPropID;
extern int gSelectedPropAreaIndex;

void CheckAreaSelection(AppState& state);
void ClearPropSelection();
bool IsPropSelected(uint32_t uniqueID);
const Prop* GetSelectedProp();
void HandleGlobalEscape(AppState& state);