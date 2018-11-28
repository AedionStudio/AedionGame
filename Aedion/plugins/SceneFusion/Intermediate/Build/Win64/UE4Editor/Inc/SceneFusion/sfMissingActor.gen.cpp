// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/Actors/sfMissingActor.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfMissingActor() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfMissingActor_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_AsfMissingActor();
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
	ENGINE_API UClass* Z_Construct_UClass_UActorComponent_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfMissingObject_NoRegister();
// End Cross Module References
	void AsfMissingActor::StaticRegisterNativesAsfMissingActor()
	{
	}
	UClass* Z_Construct_UClass_AsfMissingActor_NoRegister()
	{
		return AsfMissingActor::StaticClass();
	}
	struct Z_Construct_UClass_AsfMissingActor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_SimpleConstructionComponents_MetaData[];
#endif
		static const UE4CodeGen_Private::FArrayPropertyParams NewProp_SimpleConstructionComponents;
		static const UE4CodeGen_Private::FObjectPropertyParams NewProp_SimpleConstructionComponents_Inner;
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_ClassName_MetaData[];
#endif
		static const UE4CodeGen_Private::FStrPropertyParams NewProp_ClassName;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UE4CodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AsfMissingActor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AsfMissingActor_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "Actors/sfMissingActor.h" },
		{ "ModuleRelativePath", "Private/Actors/sfMissingActor.h" },
		{ "ToolTip", "Actor class to use as a stand-in for missing actor classes." },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents_MetaData[] = {
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Private/Actors/sfMissingActor.h" },
		{ "ToolTip", "If we set the creation method of a component to SimpleConstructionScript on a non-blueprint, the component will\nget deleted, so we store the components that should have that creation method and instead assign them the\ndefault creation method." },
	};
#endif
	const UE4CodeGen_Private::FArrayPropertyParams Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents = { UE4CodeGen_Private::EPropertyClass::Array, "SimpleConstructionComponents", RF_Public|RF_Transient|RF_MarkAsNative, (EPropertyFlags)0x0010008000000008, 1, nullptr, STRUCT_OFFSET(AsfMissingActor, SimpleConstructionComponents), METADATA_PARAMS(Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents_MetaData, ARRAY_COUNT(Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents_MetaData)) };
	const UE4CodeGen_Private::FObjectPropertyParams Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents_Inner = { UE4CodeGen_Private::EPropertyClass::Object, "SimpleConstructionComponents", RF_Public|RF_Transient|RF_MarkAsNative, (EPropertyFlags)0x0000000000080008, 1, nullptr, 0, Z_Construct_UClass_UActorComponent_NoRegister, METADATA_PARAMS(nullptr, 0) };
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AsfMissingActor_Statics::NewProp_ClassName_MetaData[] = {
		{ "Category", "sfMissingActor" },
		{ "ModuleRelativePath", "Private/Actors/sfMissingActor.h" },
		{ "ToolTip", "Name of the missing actor class" },
	};
#endif
	const UE4CodeGen_Private::FStrPropertyParams Z_Construct_UClass_AsfMissingActor_Statics::NewProp_ClassName = { UE4CodeGen_Private::EPropertyClass::Str, "ClassName", RF_Public|RF_Transient|RF_MarkAsNative, (EPropertyFlags)0x0010000000020001, 1, nullptr, STRUCT_OFFSET(AsfMissingActor, ClassName), METADATA_PARAMS(Z_Construct_UClass_AsfMissingActor_Statics::NewProp_ClassName_MetaData, ARRAY_COUNT(Z_Construct_UClass_AsfMissingActor_Statics::NewProp_ClassName_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_AsfMissingActor_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AsfMissingActor_Statics::NewProp_SimpleConstructionComponents_Inner,
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_AsfMissingActor_Statics::NewProp_ClassName,
	};
		const UE4CodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_AsfMissingActor_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UsfMissingObject_NoRegister, (int32)VTABLE_OFFSET(AsfMissingActor, IsfMissingObject), false },
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_AsfMissingActor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AsfMissingActor>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_AsfMissingActor_Statics::ClassParams = {
		&AsfMissingActor::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x008000A0u,
		nullptr, 0,
		Z_Construct_UClass_AsfMissingActor_Statics::PropPointers, ARRAY_COUNT(Z_Construct_UClass_AsfMissingActor_Statics::PropPointers),
		nullptr,
		&StaticCppClassTypeInfo,
		InterfaceParams, ARRAY_COUNT(InterfaceParams),
		METADATA_PARAMS(Z_Construct_UClass_AsfMissingActor_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_AsfMissingActor_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AsfMissingActor()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_AsfMissingActor_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(AsfMissingActor, 867692600);
	static FCompiledInDefer Z_CompiledInDefer_UClass_AsfMissingActor(Z_Construct_UClass_AsfMissingActor, &AsfMissingActor::StaticClass, TEXT("/Script/SceneFusion"), TEXT("AsfMissingActor"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(AsfMissingActor);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
