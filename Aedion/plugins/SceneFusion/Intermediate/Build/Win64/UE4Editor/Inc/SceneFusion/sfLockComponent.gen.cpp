// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/Components/sfLockComponent.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfLockComponent() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfLockComponent_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfLockComponent();
	ENGINE_API UClass* Z_Construct_UClass_USceneComponent();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
// End Cross Module References
	void UsfLockComponent::StaticRegisterNativesUsfLockComponent()
	{
	}
	UClass* Z_Construct_UClass_UsfLockComponent_NoRegister()
	{
		return UsfLockComponent::StaticClass();
	}
	struct Z_Construct_UClass_UsfLockComponent_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UsfLockComponent_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_USceneComponent,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UsfLockComponent_Statics::Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Custom" },
		{ "HideCategories", "Trigger PhysicsVolume" },
		{ "IncludePath", "Components/sfLockComponent.h" },
		{ "ModuleRelativePath", "Private/Components/sfLockComponent.h" },
		{ "ToolTip", "Lock component for indicating an actor cannot be edited. This is added to each mesh component of the actor, and\nadds a copy of the mesh as a child with a lock shader. It also deletes itself and unlocks the actor when copied." },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UsfLockComponent_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UsfLockComponent>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UsfLockComponent_Statics::ClassParams = {
		&UsfLockComponent::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x00A000A0u,
		nullptr, 0,
		nullptr, 0,
		nullptr,
		&StaticCppClassTypeInfo,
		nullptr, 0,
		METADATA_PARAMS(Z_Construct_UClass_UsfLockComponent_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_UsfLockComponent_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UsfLockComponent()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UsfLockComponent_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UsfLockComponent, 2297665377);
	static FCompiledInDefer Z_CompiledInDefer_UClass_UsfLockComponent(Z_Construct_UClass_UsfLockComponent, &UsfLockComponent::StaticClass, TEXT("/Script/SceneFusion"), TEXT("UsfLockComponent"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UsfLockComponent);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
