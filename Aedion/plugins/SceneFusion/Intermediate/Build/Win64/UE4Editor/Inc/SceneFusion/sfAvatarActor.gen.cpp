// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/Actors/sfAvatarActor.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfAvatarActor() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfAvatarActor_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfAvatarActor();
	ENGINE_API UClass* Z_Construct_UClass_AStaticMeshActor();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
// End Cross Module References
	void AsfAvatarActor::StaticRegisterNativesAsfAvatarActor()
	{
	}
	UClass* Z_Construct_UClass_AsfAvatarActor_NoRegister()
	{
		return AsfAvatarActor::StaticClass();
	}
	struct Z_Construct_UClass_AsfAvatarActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AsfAvatarActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AStaticMeshActor,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AsfAvatarActor_Statics::Class_MetaDataParams[] = {
		{ "HideCategories", "Input" },
		{ "IncludePath", "Actors/sfAvatarActor.h" },
		{ "ModuleRelativePath", "Private/Actors/sfAvatarActor.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
		{ "ToolTip", "Unselectable static meshPtr actor used for avatars." },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AsfAvatarActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AsfAvatarActor>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_AsfAvatarActor_Statics::ClassParams = {
		&AsfAvatarActor::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x008000A0u,
		nullptr, 0,
		nullptr, 0,
		nullptr,
		&StaticCppClassTypeInfo,
		nullptr, 0,
		METADATA_PARAMS(Z_Construct_UClass_AsfAvatarActor_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_AsfAvatarActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AsfAvatarActor()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_AsfAvatarActor_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(AsfAvatarActor, 1138155454);
	static FCompiledInDefer Z_CompiledInDefer_UClass_AsfAvatarActor(Z_Construct_UClass_AsfAvatarActor, &AsfAvatarActor::StaticClass, TEXT("/Script/SceneFusion"), TEXT("AsfAvatarActor"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(AsfAvatarActor);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
