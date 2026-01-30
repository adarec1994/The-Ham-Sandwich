#pragma once
#include "UI.h"
#include "../Area/Props.h"
#include <set>

extern uint32_t gSelectedPropID;
extern int gSelectedPropAreaIndex;
extern std::set<uint32_t> gHiddenProps;
extern std::set<uint32_t> gDeletedProps;
extern int gSelectedSkyModelIndex;

void CheckAreaSelection(AppState& state);
void ClearPropSelection();
void SelectProp(uint32_t uniqueID, int areaIndex);
bool IsPropSelected(uint32_t uniqueID);
bool IsPropHidden(uint32_t uniqueID);
bool IsPropDeleted(uint32_t uniqueID);
bool IsPropVisible(uint32_t uniqueID);
const Prop* GetSelectedProp();
void HandleGlobalKeys(AppState& state);
void HideSelectedProp();
void ShowAllHiddenProps();
void DeleteSelectedProp();

void SelectSkyModel(int index);
void ClearSkyModelSelection();
bool IsSkyModelSelected(int index);
void HideSelectedSkyModel();
void ShowAllHiddenSkyModels();
void DeleteSelectedSkyModel();
bool IsSkyModelHidden(int index);
bool IsSkyModelDeleted(int index);
bool IsSkyModelVisible(int index);