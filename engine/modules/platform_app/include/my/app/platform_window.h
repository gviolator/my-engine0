// #my_engine_source_file

#pragma once

#include <string_view>
#include <vectormath/vec2d.hpp>

#include "my/rtti/rtti_object.h"


namespace my
{
    struct MY_ABSTRACT_TYPE IPlatformWindow : virtual IRefCounted
    {
        MY_INTERFACE(my::IPlatformWindow, IRefCounted)

        virtual void setVisible(bool) = 0;

        virtual bool isVisible() const = 0;

        virtual Vectormath::IVector2 getSize() const = 0;

        virtual void setSize(Vectormath::IVector2) = 0;

        virtual Vectormath::IVector2 getClientSize() const = 0;

        virtual void setPosition(Vectormath::IVector2) = 0;

        virtual Vectormath::IVector2 getPosition() const = 0;

        virtual void setName(std::string_view) = 0;
    };

}  // namespace my
