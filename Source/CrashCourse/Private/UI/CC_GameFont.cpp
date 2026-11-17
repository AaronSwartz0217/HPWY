#include "UI/CC_GameFont.h"
#include "Fonts/CompositeFont.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"

FSlateFontInfo CCGameFont::Kai(const FSlateFontInfo& Original)
{
#if PLATFORM_WINDOWS
 static const TSharedPtr<const FCompositeFont> Font = []() -> TSharedPtr<const FCompositeFont>
 {
  const FString WindowsPath = FPlatformMisc::GetEnvironmentVariable(TEXT("WINDIR"));
  if (WindowsPath.IsEmpty()) return nullptr;
  const FString FontPath = FPaths::Combine(WindowsPath, TEXT("Fonts"), TEXT("simkai.ttf"));
  if (!FPaths::FileExists(FontPath)) return nullptr;
  return MakeShared<FCompositeFont>(NAME_None, FontPath, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
 }();

 if (Font)
 {
  FSlateFontInfo Result(Font, Original.Size, NAME_None, Original.OutlineSettings);
  Result.LetterSpacing = Original.LetterSpacing;
  return Result;
 }
#endif
 return Original;
}
