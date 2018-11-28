#pragma once

#include "sfValueProperty.h"

using namespace KS;
using namespace KS::SceneFusion2;

/**
 * Utility for converting between SF properties and basic types.
 */
namespace KS
{
namespace SceneFusion2
{
    /**
     * Converts a property to an int. If the propertyPtr is a null pointer or cannot downcast to a sfValueProperty,
     * then an invalid_argument exception will be thrown.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  int
     */
    inline int ToInt(sfProperty::SPtr propertyPtr)
    {
        return propertyPtr->AsValue()->GetValue().GetInt();
    }

    /**
     * Converts a property to an uint. If the propertyPtr is a null pointer or cannot downcast to a sfValueProperty,
     * then an invalid_argument exception will be thrown.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  uint32_t
     */
    inline uint32_t ToUInt(sfProperty::SPtr propertyPtr)
    {
        return propertyPtr->AsValue()->GetValue().GetUInt();
    }

    /**
     * Converts a property to a float. If the propertyPtr is a null pointer or cannot downcast to a sfValueProperty,
     * then an invalid_argument exception will be thrown.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  float
     */
    inline float ToFloat(sfProperty::SPtr propertyPtr)
    {
        return propertyPtr->AsValue()->GetValue().GetFloat();
    }

    /**
     * Converts a property to a bool. If the propertyPtr is a null pointer or cannot downcast to a sfValueProperty,
     * then an invalid_argument exception will be thrown.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  bool
     */
    inline bool ToBool(sfProperty::SPtr propertyPtr)
    {
        return propertyPtr->AsValue()->GetValue().GetBool();
    }

    /**
     * Converts a property to a byte. If the propertyPtr is a null pointer or cannot downcast to a sfValueProperty,
     * then an invalid_argument exception will be thrown.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  uint8_t
     */
    inline uint8_t ToByte(sfProperty::SPtr propertyPtr)
    {
        return propertyPtr->AsValue()->GetValue().GetByte();
    }

    /**
     * Converts a property to a long. If the propertyPtr is a null pointer or cannot downcast to a sfValueProperty,
     * then an invalid_argument exception will be thrown.
     *
     * @param   sfProperty::SPtr propertyPtr
     * @return  int64_t
     */
    inline int64_t ToLong(sfProperty::SPtr propertyPtr)
    {
        return propertyPtr->AsValue()->GetValue().GetLong();
    }
}//SceneFusion
}//KS
