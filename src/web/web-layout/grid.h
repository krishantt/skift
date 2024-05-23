#pragma once

#include "base.h"

namespace Web::Layout {

struct GridFlow : public Flow {
    static constexpr auto TYPE = GRID;

    Type type() const override {
        return TYPE;
    }
};

} // namespace Web::Layout
