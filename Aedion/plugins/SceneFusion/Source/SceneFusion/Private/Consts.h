#pragma once

#include <sfName.h>

using namespace KS::SceneFusion2;

/**
 * Property names
 */
class sfProp
{
public:
    static const sfName Location;
    static const sfName Rotation;
    static const sfName Scale;
    static const sfName Name;
    static const sfName Class;
    static const sfName Label;
    static const sfName Folder;
    static const sfName Mesh;
    static const sfName Id;
    static const sfName Flashlight;
    static const sfName Level;
    static const sfName IsPersistentLevel;
    static const sfName WorldComposition;
    static const sfName LevelPropertyId;
    static const sfName PackageName;
    static const sfName ParentPackageName;
    static const sfName TilePosition;
    static const sfName CreationMethod;
    static const sfName Visualize;
    static const sfName IsRoot;
    static const sfName Flags;
    static const sfName DefaultGameMode;
    static const sfName HierarchicalLODSetup;
};

/**
 * Object types
 */
class sfType
{
public:
    static const sfName Actor;
    static const sfName Avatar;
    static const sfName Level;
    static const sfName LevelLock;
    static const sfName LevelProperties;
    static const sfName Component;
    static const sfName MeshBounds;
    static const sfName GameMode;
};