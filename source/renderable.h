#ifndef SHIPZ_RENDERABLE_H
#define SHIPZ_RENDERABLE_H

class Renderable {
    public:
    virtual ~Renderable() = default;
    // Draw this renderable
    virtual void Draw() = 0;
};

#endif