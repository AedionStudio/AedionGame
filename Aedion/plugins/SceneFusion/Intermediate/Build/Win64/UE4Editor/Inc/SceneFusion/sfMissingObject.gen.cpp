// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/sfMissingObject.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfMissingObject() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfMissingObject_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfMissingObject();
	COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
// End Cross Module References
	void UsfMissingObject::StaticRegisterNativesUsfMissingObject()
	{
	}
	UClass* Z_Construct_UClass_UsfMissingObject_NoRegister()
	{
		return UsfMissingObject::StaticClass();
	}
	struct Z_Construct_UClass_UsfMissingObject_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UsfMissingObject_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UsfMissingObject_Statics::Class_MetaDataParams[] = {
		{ "ModuleRelativePath", "Private/sfMissingObject.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UsfMissingObject_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IsfMissingObject>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UsfMissingObject_Statics::ClassParams = {
		&UsfMissingObject::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x000840A1u,
		nullptr, 0,
		nullptr, 0,
		nullptr,
		&StaticCppClassTypeInfo,
		nullptr, 0,
		METADATA_PARAMS(Z_Construct_UClass_UsfMissingObject_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_UsfMissingObject_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UsfMissingObject()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UsfMissingObject_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UsfMissingObject, 1342070359);
	static FCompiledInDefer Z_CompiledInDefer_UClass_UsfMissingObject(Z_Construct_UClass_UsfMissingObject, &UsfMissingObject::StaticClass, TEXT("/Script/SceneFusion"), TEXT("UsfMissingObject"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UsfMissingObject);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
