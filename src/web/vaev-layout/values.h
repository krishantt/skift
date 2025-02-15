#pragma once

#include <karm-text/font.h>
#include <vaev-base/color.h>
#include <vaev-base/font.h>
#include <vaev-base/width.h>

#include "base.h"

namespace Vaev::Layout {

template <typename T>
concept Resolvable = requires {
    typename T::Resolved;
};

template <typename T>
using Resolved = Meta::Cond<Resolvable<T>, typename T::Resolved, T>;
static_assert(Resolvable<PercentOr<Length>>);

struct Resolver {
    f64 userFontSize = 16;   /// Font size of the user agent
    f64 parentFontSize = 16; /// Font size of the parent box

    Opt<Text::Font> rootFont = NONE;                 /// Font of the root element
    Opt<Text::Font> boxFont = NONE;                  /// Font of the current box
    Viewport viewport = {.small = {800_px, 600_px}}; /// Viewport of the current box
    Axis boxAxis = Axis::HORIZONTAL;                 /// Inline axis of the current box

    static Resolver from(Tree const& tree, Box const& box);

    Resolver inherit(Resolver const& resolver);

    Px _resolveFontRelative(Length const& value);

    Px resolve(Length const& value);

    Px resolve(PercentOr<Length> const& value, Px relative);

    Px resolve(Width const& value, Px relative);

    Px resolve(FontSize const& value);

    // MARK: Eval --------------------------------------------------------------

    template <typename T>
    Resolved<T> _resolveInfix(typename CalcValue<T>::OpCode op, Resolved<T> lhs, Resolved<T> rhs) {
        switch (op) {
        case CalcValue<T>::OpCode::ADD:
            return lhs + rhs;
        case CalcValue<T>::OpCode::SUBSTRACT:
            return lhs - rhs;
        case CalcValue<T>::OpCode::MULTIPLY:
            return lhs * rhs;
        case CalcValue<T>::OpCode::DIVIDE:
            return lhs / rhs;
        default:
            return lhs;
        }
    }

    template <typename T, typename... Args>
    auto resolve(CalcValue<T> const& value, Args... args) -> Resolved<T> {
        if (value.type == CalcValue<T>::OpType::FIXED) {
            return resolve(value.lhs.template unwrap<T>(), args...);
        } else if (value.type == CalcValue<T>::OpType::SINGLE) {
            // TODO: compute result of funtion here with the resolved value
            return resolve(value.lhs.template unwrap<T>(), args...);
        } else if (value.type == CalcValue<T>::OpType::CALC) {
            auto resolveUnion = Visitor{
                [&](T const& v) {
                    return resolve<T>(v, args...);
                },
                [&](CalcValue<T>::Leaf const& v) {
                    return resolve<T>(*v, args...);
                },
                [&](Number const& v) {
                    return Math::i24f8{v};
                },
                [&](None const&) -> Resolved<T> {
                    panic("invalid value in calc expression");
                }
            };

            return _resolveInfix<T>(
                value.op,
                value.lhs.visit(resolveUnion),
                value.rhs.visit(resolveUnion)
            );
        }

        unreachable();
    }
};

// MARK: Resolve during layout -------------------------------------------------

Px resolve(Tree const& tree, Box const& box, Length const& value);

Px resolve(Tree const& tree, Box const& box, PercentOr<Length> const& value, Px relative);

Px resolve(Tree const& tree, Box const& box, Width const& value, Px relative);

Px resolve(Tree const& tree, Box const& box, FontSize const& value);

template <typename T, typename... Args>
static inline auto resolve(Tree const& tree, Box const& box, CalcValue<T> const& value, Args... args) -> Resolved<T> {
    return Resolver::from(tree, box).resolve(value, args...);
}

} // namespace Vaev::Layout
