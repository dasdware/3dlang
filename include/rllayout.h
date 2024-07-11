#ifndef __DW_RLLAYOUT_H
#define __DW_RLLAYOUT_H

#define RLLAYOUT_STACK_CAPACITY 32

#define WHICH_DEFAULT 0
#define WHICH_REMAINING -1
#define WHICH_ANCHORED -2

#include <raylib.h>
#include <error.h>

typedef enum
{
    LAYOUT_SCREEN,
    LAYOUT_ANCHOR,
    LAYOUT_STACK,
    LAYOUT_SPACED,
    LAYOUT_RECTANGLE,
} DW_RLLayoutKind;

typedef enum
{
    ANCHOR_TOP,
    ANCHOR_BOTTOM,
    ANCHOR_LEFT,
    ANCHOR_RIGHT,
} DW_RLLayoutAnchorKind;

typedef enum
{
    DIRECTION_VERTICAL,
    DIRECTION_HORIZONTAL,
} DW_RLLayoutDirection;

typedef struct
{
    int padding;
} DW_RLLayoutPaddingData;

typedef struct
{
    DW_RLLayoutAnchorKind kind;
    int size;
    int gap;
} DW_RLLayoutAnchorData;

typedef struct
{
    DW_RLLayoutDirection direction;
    int item_size;
    int gap;
    int cursor;
} DW_RLLayoutStackData;

typedef struct
{
    DW_RLLayoutDirection direction;
    int count;
    int cursor;
} DW_RLLayoutSpacedData;

typedef union
{
    DW_RLLayoutPaddingData screen;
    DW_RLLayoutAnchorData anchored;
    DW_RLLayoutStackData stack;
    DW_RLLayoutSpacedData spaced;
    Rectangle rectangle;
} DW_RLLayoutAs;

typedef struct
{
    DW_RLLayoutKind kind;
    Rectangle parent_bounds;
    DW_RLLayoutAs as;
} DW_RLLayout;

Rectangle rl_padding(Rectangle bounds, int left, int top, int right, int bottom);
#define rl_padding_all(bounds, padding) rl_padding(bounds, padding, padding, padding, padding)
#define rl_padding_symmetric(bounds, horizontal, vertical) rl_padding(bounds, horizontal, vertical, horizontal, vertical)

Rectangle rl_center(Rectangle bounds, int width, int height);

void rl_spacing(int amount);

Rectangle rl_rectangle_which(int which);
#define rl_rectangle() rl_rectangle_which(WHICH_DEFAULT)

void rl_begin_screen(int padding);
void rl_begin_anchor(int which, DW_RLLayoutAnchorKind kind, int size, int gap);
void rl_begin_stack(int which, DW_RLLayoutDirection direction, int item_size, int gap);
void rl_begin_spaced(int which, DW_RLLayoutDirection direction, int count);
void rl_begin_rectangle(int which, Rectangle rectangle);
void rl_end();

#endif // __DW_RLLAYOUT_H

#ifdef RLLAYOUT_IMPLEMENTATION

#include <nob.h>

static DW_RLLayout rl_layout_stack[RLLAYOUT_STACK_CAPACITY] = {0};
static size_t rl_layout_stack_count = 0;

Rectangle rl_padding(Rectangle bounds, int left, int top, int right, int bottom)
{
    Rectangle result = bounds;
    result.x += left;
    result.y += top;
    result.width -= left + right;
    result.height -= top + bottom;
    return result;
}

Rectangle rl_center(Rectangle bounds, int width, int height)
{
    Rectangle result = {0};
    result.x = bounds.x + (bounds.width - width) / 2;
    result.y = bounds.y + (bounds.height - height) / 2;
    result.width = width;
    result.height = height;
    return result;
}

Rectangle rl_rectangle_which(int which)
{
    NOB_ASSERT(rl_layout_stack_count > 0);

    DW_RLLayout* layout = &rl_layout_stack[rl_layout_stack_count - 1];
    Rectangle result = layout->parent_bounds;

    switch (layout->kind)
    {
    case LAYOUT_SCREEN:
        result = CLITERAL(Rectangle) {
            .x = 0,
            .y = 0,
            .width = GetScreenWidth(),
            .height = GetScreenHeight(),
        };
        result = rl_padding_all(result, layout->as.screen.padding);
        break;
    case LAYOUT_ANCHOR:
    {
        size_t size = layout->as.anchored.size;
        if (which == WHICH_ANCHORED)
        {
            switch (layout->as.anchored.kind)
            {
            case ANCHOR_TOP:
                result.height = size;
                break;
            case ANCHOR_LEFT:
                result.height = size;
                break;
            case ANCHOR_BOTTOM:
                result.y += result.height - size;
                result.height = size;
                break;
            case ANCHOR_RIGHT:
                result.x += result.width - size;
                result.width = size;
                break;
            default:
                DW_UNIMPLEMENTED_MSG("Unknown anchor kind `%d`.", layout->as.anchored.kind);
            }
        }
        else if (which == WHICH_DEFAULT || which == WHICH_REMAINING)
        {
            int gap = layout->as.anchored.gap;
            switch (layout->as.anchored.kind)
            {
            case ANCHOR_TOP:
                result.y += size + gap;
                result.height -= size + gap;
                break;
            case ANCHOR_LEFT:
                result.x += size + gap;
                result.width -= size + gap;
                break;
            case ANCHOR_BOTTOM:
                result.height -= size + gap;
                break;
            case ANCHOR_RIGHT:
                result.width -= size + gap;
                break;
            default:
                DW_UNIMPLEMENTED_MSG("Unknown anchor kind `%d`.", layout->as.anchored.kind);
            }
        }
        else
        {
            DW_UNIMPLEMENTED_MSG("Unknown anchor which `%d`.", which);
        }
        break;
    }
    case LAYOUT_STACK:
    {
        int item_size = layout->as.stack.item_size;

        switch (layout->as.stack.direction)
        {
        case DIRECTION_HORIZONTAL:
            result.x += layout->as.stack.cursor;
            result.width = item_size;
            break;
        case DIRECTION_VERTICAL:
            result.y += layout->as.stack.cursor;
            result.height = item_size;
            break;
        default:
            DW_UNIMPLEMENTED_MSG("Unknown stack direction `%d`.", layout->as.stack.direction);
        }

        layout->as.stack.cursor += item_size + layout->as.stack.gap;
        break;
    }
    case LAYOUT_SPACED:
    {
        float fract = 1.0 / layout->as.spaced.count;
        float begin = layout->as.spaced.cursor * fract;
        float size = which * fract;

        switch (layout->as.spaced.direction)
        {
        case DIRECTION_HORIZONTAL:
        {
            size_t s = result.width;
            result.x = result.x + s * begin;
            result.width = s * size;
            break;
        }
        case DIRECTION_VERTICAL:
        {
            size_t s = result.height;
            result.y = result.y + s * begin;
            result.height = s * size;
            break;
        }
        default:
            DW_UNIMPLEMENTED_MSG("Unknown spaced direction `%d`.", layout->as.stack.direction);
        }

        layout->as.spaced.cursor += which;
        break;
    }
    case LAYOUT_RECTANGLE:
        result = layout->as.rectangle;
        break;
    default:
        DW_UNIMPLEMENTED_MSG("Unknown layout kind `%d`.", layout->kind);
    }

    return result;
}

void _rl_begin(DW_RLLayout layout)
{
    NOB_ASSERT(rl_layout_stack_count < RLLAYOUT_STACK_CAPACITY);
    rl_layout_stack[rl_layout_stack_count] = layout;
    ++rl_layout_stack_count;
}

void rl_begin_screen(int padding)
{
    DW_RLLayout layout = {
        .kind = LAYOUT_SCREEN,
        .parent_bounds = {0},
        .as.screen.padding = padding,
    };
    _rl_begin(layout);
}

void rl_begin_anchor(int which, DW_RLLayoutAnchorKind kind, int size, int gap)
{
    DW_RLLayout layout = {
        .kind = LAYOUT_ANCHOR,
        .parent_bounds = rl_rectangle_which(which),
        .as.anchored = {
            .kind = kind,
            .size = size,
            .gap = gap,
        },
    };
    _rl_begin(layout);
}

void rl_begin_stack(int which, DW_RLLayoutDirection direction, int item_size, int gap)
{
    DW_RLLayout layout = {
        .kind = LAYOUT_STACK,
        .parent_bounds = rl_rectangle_which(which),
        .as.stack = {
            .direction = direction,
            .item_size = item_size,
            .gap = gap,
            .cursor = 0,
        },
    };
    _rl_begin(layout);
}

void rl_begin_spaced(int which, DW_RLLayoutDirection direction, int count)
{
    DW_RLLayout layout = {
        .kind = LAYOUT_SPACED,
        .parent_bounds = rl_rectangle_which(which),
        .as.spaced = {
            .direction = direction,
            .count = count,
            .cursor = 0,
        },
    };
    _rl_begin(layout);
}

void rl_begin_rectangle(int which, Rectangle rectangle)
{
    DW_RLLayout layout = {
        .kind = LAYOUT_RECTANGLE,
        .parent_bounds = rl_rectangle_which(which),
        .as.rectangle = rectangle,
    };
    _rl_begin(layout);
}

void rl_spacing(int amount)
{
    DW_RLLayout* layout = &rl_layout_stack[rl_layout_stack_count - 1];
    if (layout->kind == LAYOUT_STACK)
    {
        layout->as.stack.cursor += amount;
    }
}

void rl_end()
{
    NOB_ASSERT(rl_layout_stack_count > 0);
    rl_layout_stack_count--;
}

#endif // RLLAYOUT_IMPLEMENTATION