#pragma once

#include <CoreMinimal.h>
#include <Styling/SlateStyle.h>

class sfUIStyles
{
public:
    /**
     * Initialize.
     */
    static void Initialize();

    /**
     * Shutdown.
     */
    static void Shutdown();

    /**
     * Style name.
     *
     * @return  FName
     */
    static FName GetStyleSetName();

    /**
     * Reload slate textures.
     */
    static void ReloadTextures();

    /**
     * Return the slate style set.
     *
     * @return  const ISlateStyle&
     */
    static const ISlateStyle& Get();

    /**
     * Create an icon brush.
     *
     * @param   const FString& iconName
     * @param   const FVector2D& iconSize
     * @return  FSlateImageBrush*
     */
    static FSlateImageBrush* CreateIcon(const FString& iconName, const FVector2D& iconSize);

    /**
     * Get default font style.
     *
     * @param   const FName typefaceFontName
     * @param   const int size
     * @return  FSlateFontInfo
     */
    static FSlateFontInfo GetDefaultFontStyle(const FName typefaceFontName, const int size);

private:
    static TSharedPtr<class FSlateStyleSet> StyleInstancePtr;

    /**
     * Create the slate styles.
     *
     * @return TSharedRef<FSlateStyleSet>
     */
    static TSharedRef<class FSlateStyleSet> Create();
};