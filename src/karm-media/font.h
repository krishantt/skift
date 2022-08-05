#pragma once

#include <karm-base/rc.h>
#include <karm-math/rect.h>

namespace Karm::Gfx {
struct Context;
};

namespace Karm::Media {

enum FontWeight {
    THIN = 100,
    EXTRA_LIGHT = 200,
    LIGHT = 300,
    REGULAR = 400,
    MEDIUM = 500,
    SEMI_BOLD = 600,
    BOLD = 700,
    EXTRA_BOLD = 800,
    BLACK = 900,
};

struct FontAttrs {
    int size;
    FontWeight weight;
    bool italic;
    bool monospace;
};

/**
                                                        <- lineAscend
   _____       _             _                          <- ascend
  / ____|     | |           | |                         <- captop
 | |  __ _   _| |_ ___ _ __ | |__   ___ _ __ __ _
 | | |_ | | | | __/ _ \ '_ \| '_ \ / _ \ '__/ _` |
 | |__| | |_| | ||  __/ | | | |_) |  __/ | | (_| |
  \_____|\__,_|\__\___|_| |_|_.__/ \___|_|  \__, |      <- baseline (origin)
                                             __/ |
                                            |___/       <- descend
 | ---- |                                               ...advance
                                                        <- lineDescend
*/
struct FontMetrics {
    double lineAscend;
    double ascend;
    double captop;
    double descend;
    double lineDescend;
    double advance;
};

struct FontMesure {
    Math::Rectf capbound;
    Math::Rectf linebound;
    Math::Vec2f baseline;
};

struct Font {
    static Strong<Font> fallback();

    virtual ~Font() = default;

    virtual FontMetrics metrics() const = 0;
    virtual double advance(Rune c) const = 0;
    virtual void fillRune(Gfx::Context &ctx, Math::Vec2i baseline, Rune rune) const = 0;
    virtual void strokeRune(Gfx::Context &ctx, Math::Vec2i baseline, Rune rune) const {
        fillRune(ctx, baseline, rune);
    }

    FontMesure mesureRune(Rune r) const {
        auto adv = advance(r);
        auto m = metrics();

        return {
            .capbound = {adv, m.ascend + m.descend},
            .linebound = {adv, m.lineAscend + m.lineDescend},
            .baseline = {0, m.ascend},
        };
    }

    FontMesure mesureStr(Str str) const {
        double adv = 0;
        for (auto r : iterRunes(str)) {
            adv += advance(r);
        }

        auto m = metrics();
        return {
            .capbound = {adv, m.ascend + m.descend},
            .linebound = {adv, m.lineAscend + m.lineDescend},
            .baseline = {0, m.ascend},
        };
    }

    void fillStr(Gfx::Context &ctx, Math::Vec2i baseline, Str str) const {
        for (auto r : iterRunes(str)) {
            fillRune(ctx, baseline, r);
            baseline.x += advance(r);
        }
    }

    void strokeStr(Gfx::Context &ctx, Math::Vec2i baseline, Str str) const {
        for (auto r : iterRunes(str)) {
            strokeRune(ctx, baseline, r);
            baseline.x += advance(r);
        }
    }
};

} // namespace Karm::Media
