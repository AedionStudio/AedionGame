// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "SceneFusion/Private/Objects/sfUndoHook.h"
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4883)
#endif
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodesfUndoHook() {}
// Cross Module References
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfUndoHook_NoRegister();
	SCENEFUSION_API UClass* Z_Construct_UClass_UsfUndoHook();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	UPackage* Z_Construct_UPackage__Script_SceneFusion();
// End Cross Module References
	void UsfUndoHook::StaticRegisterNativesUsfUndoHook()
	{
	}
	UClass* Z_Construct_UClass_UsfUndoHook_NoRegister()
	{
		return UsfUndoHook::StaticClass();
	}
	struct Z_Construct_UClass_UsfUndoHook_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UE4CodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UE4CodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UsfUndoHook_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_SceneFusion,
	};
#if WITH_METADATA
	const UE4CodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UsfUndoHook_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "Objects/sfUndoHook.h" },
		{ "ModuleRelativePath", "Private/Objects/sfUndoHook.h" },
		{ "ToolTip", "This is part of a hack to run code after an undo transaction but before the objects in the transaction have\nPostEditUndo called." },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UsfUndoHook_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UsfUndoHook>::IsAbstract,
	};
	const UE4CodeGen_Private::FClassParams Z_Construct_UClass_UsfUndoHook_Statics::ClassParams = {
		&UsfUndoHook::StaticClass,
		DependentSingletons, ARRAY_COUNT(DependentSingletons),
		0x000000A0u,
		nullptr, 0,
		nullptr, 0,
		nullptr,
		&StaticCppClassTypeInfo,
		nullptr, 0,
		METADATA_PARAMS(Z_Construct_UClass_UsfUndoHook_Statics::Class_MetaDataParams, ARRAY_COUNT(Z_Construct_UClass_UsfUndoHook_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UsfUndoHook()
	{
		static UClass* OuterClass = nullptr;
		if (!OuterClass)
		{
			UE4CodeGen_Private::ConstructUClass(OuterClass, Z_Construct_UClass_UsfUndoHook_Statics::ClassParams);
		}
		return OuterClass;
	}
	IMPLEMENT_CLASS(UsfUndoHook, 1728341343);
	static FCompiledInDefer Z_CompiledInDefer_UClass_UsfUndoHook(Z_Construct_UClass_UsfUndoHook, &UsfUndoHook::StaticClass, TEXT("/Script/SceneFusion"), TEXT("UsfUndoHook"), false, nullptr, nullptr, nullptr);
	DEFINE_VTABLE_PTR_HELPER_CTOR(UsfUndoHook);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef _MSC_VER
#pragma warning (pop)
#endif
