#pragma once

#include "stack.h"

namespace Karm::Scene {

struct Page : public Stack {
    void print(Print::Printer &doc) override {
        Stack::print(doc);
        paint(doc.beginPage());
    }

    void repr(Io::Emit &e) const override {
        e("(page");
        if (_children) {
            e.indentNewline();
            for (auto &child : _children) {
                child->repr(e);
                e.newline();
            }
            e.deindent();
        }
        e(")");
    }
};

} // namespace Karm::Scene
