#pragma once
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateTypes.h"

class UTexture2D;

// Shared by the live viewport and MoviePlayer; movie attributes read atomics only.
class SCC_LoadingScreen : public SCompoundWidget
{
public:
 SLATE_BEGIN_ARGS(SCC_LoadingScreen) : _Logo(nullptr), _Opening(false) {}
  SLATE_ARGUMENT(UTexture2D*, Logo)
  SLATE_ATTRIBUTE(FText, Title)
  SLATE_ATTRIBUTE(TOptional<float>, Progress)
  SLATE_ATTRIBUTE(bool, Opening)
 SLATE_END_ARGS()
 void Construct(const FArguments& Args);
private:
 FSlateBrush LogoBrush;
 FProgressBarStyle ProgressStyle;
};
