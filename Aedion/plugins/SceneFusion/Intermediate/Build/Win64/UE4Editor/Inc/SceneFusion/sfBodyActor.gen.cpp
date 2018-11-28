// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/Actors/sfBodyActor.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfBodyActor() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfBodyActor_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfBodyActor();
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfAvatarActor();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
// End Cross Module References
	void AsfBodyActor::StaticRegisterNativesAsfBodyActor()
	{
	}
	UClass* Z_Construct_UClass_AsfBodyActor_NoRegister()
	{
		return AsfBodyActor::StaticClass();
	}
	struct Z_Construct_UClass_AsfBodyActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AsfBodyActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AsfAvatarActor,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AsfBodyActor_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Input" },
		{ "IncludePath", "Actors/sfBodyActor.h" },
		{ "ModuleRelativePath", "Private/Actors/sfBodyActor.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
		{ "ToolTip", "Actor class used for XR body avatar. It manages the body's transform to follow the head naturally." },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AsfBodyActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AsfBodyActor>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_AsfBodyActor_Statics::ClassParams = {
		&AsfBodyActor::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x008000A0u,
		nullptr, 0,
		nullptr, 0,
		nullptr,
		&StaticCppClassTypeInfo,
		nullptr, 0,
		METADATA_PARAMS(Z_Construct_UClass_AsfBodyActor_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_AsfBodyActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AsfBodyActor()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_AsfBodyActor_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(AsfBodyActor, 1947282288);
	static FCompiledInDefer Z_CompiledInDefer_UClass_AsfBodyActor(Z_Construct_UClass_AsfBodyActor, &AsfBodyActor::StaticClass, TEXT("/Script/SceneFusion"), TEXT("AsfBodyActor"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(AsfBodyActor);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
