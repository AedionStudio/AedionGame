// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/Components/sfMissingComponent.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfMissingComponent() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfMissingComponent_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfMissingComponent();
	ENGINE_API UClass* Z_Construct_UClass_UActorComponent();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfMissingObject_NoRegister();
// End Cross Module References
	void UsfMissingComponent::StaticRegisterNativesUsfMissingComponent()
	{
	}
	UClass* Z_Construct_UClass_UsfMissingComponent_NoRegister()
	{
		return UsfMissingComponent::StaticClass();
	}
	struct Z_Construct_UClass_UsfMissingComponent_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam NewProp_ClassName_MetaData[];
#endif
		static const UE4CodeGen_Private::FStrPropertyParams NewProp_ClassName;
		static const UE4CodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UE4CodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UsfMissingComponent_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UActorComponent,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UsfMissingComponent_Statics::Class_MetaDataParams[] = {
		{ "BlueprintSpawnableComponent", "" },
		{ "ClassGroupNames", "Custom" },
		{ "IncludePath", "Components/sfMissingComponent.h" },
		{ "ModuleRelativePath", "Private/Components/sfMissingComponent.h" },
		{ "ToolTip", "Component class to use as a stand-in for missing non-scene component classes." },
	};
#endif
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UsfMissingComponent_Statics::NewProp_ClassName_MetaData[] = {
		{ "Category", "sfMissingComponent" },
		{ "ModuleRelativePath", "Private/Components/sfMissingComponent.h" },
		{ "ToolTip", "Name of the missing component class" },
	};
#endif
	const UE4CodeGen_Private::FStrPropertyParams Z_Construct_UClass_UsfMissingComponent_Statics::NewProp_ClassName = { UE4CodeGen_Private::EPropertyClass::Str, "ClassName", RF_Public|RF_Transient|RF_MarkAsNative, (EPropertyFlags)0x0010000000020001, 1, nullptr, STRUCT_OFFSET(UsfMissingComponent, ClassName), METADATA_PARAMS(Z_Construct_UClass_UsfMissingComponent_Statics::NewProp_ClassName_MetaData, ARRAY_COUNT(Z_Construct_UClass_UsfMissingComponent_Statics::NewProp_ClassName_MetaData)) };
	const UE4CodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UsfMissingComponent_Statics::PropPointers[] = {
		(const UE4CodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UsfMissingComponent_Statics::NewProp_ClassName,
	};
		const UE4CodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_UsfMissingComponent_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UsfMissingObject_NoRegister, (int32)VTABLE_OFFSET(UsfMissingComponent, IsfMissingObject), false },
		};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UsfMissingComponent_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UsfMissingComponent>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UsfMissingComponent_Statics::ClassParams = {
		&UsfMissingComponent::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x00A000A0u,
		nullptr, 0,
		Z_Construct_UClass_UsfMissingComponent_Statics::PropPointers, ARRAY_COUNT(Z_Construct_UClass_UsfMissingComponent_Statics::PropPointers),
		nullptr,
		&StaticCppClassTypeInfo,
		InterfaceParams, ARRAY_COUNT(InterfaceParams),
		METADATA_PARAMS(Z_Construct_UClass_UsfMissingComponent_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_UsfMissingComponent_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UsfMissingComponent()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UsfMissingComponent_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UsfMissingComponent, 2217502789);
	static FCompiledInDefer Z_CompiledInDefer_UClass_UsfMissingComponent(Z_Construct_UClass_UsfMissingComponent, &UsfMissingComponent::StaticClass, TEXT("/Script/SceneFusion"), TEXT("UsfMissingComponent"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UsfMissingComponent);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
