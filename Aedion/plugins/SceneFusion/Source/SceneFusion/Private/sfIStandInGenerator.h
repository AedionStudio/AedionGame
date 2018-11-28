#pragma once

#include <CoreMinimal.h>

/**
 * Interface for missing asset stand-in generators.
 */
class sfIStandInGenerator
{
public:
    /**
     * Generates stand-in data for a missing asset.
     *
     * @param   const FString& path of missing asset.
     * @param   UObject* uobjPtr stand-in to generate data for.
     */
    virtual void Generate(const FString& path, UObject* uobjPtr) = 0;
};