#pragma once

#include <string>
#include <vector>

namespace noctis::md {

enum class BlockType {
    Paragraph,
    Heading,
    CodeBlock,
    Quote,
    ListItem,
    Table,
    Html,
};

struct Block {
    BlockType type;
    int level = 0; // usado por Heading (1-6)
    std::string text;
};

struct Document {
    std::vector<Block> blocks;
};

} // namespace noctis::md
