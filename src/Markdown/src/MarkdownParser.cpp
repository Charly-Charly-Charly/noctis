#include "Markdown/MarkdownParser.h"

#include <md4c.h>

namespace noctis::md {

namespace {

struct ParseContext {
    Document* document;
    int pendingHeadingLevel = 0;
};

int enterBlock(MD_BLOCKTYPE type, void* detail, void* userdata) {
    auto* ctx = static_cast<ParseContext*>(userdata);
    BlockType blockType = BlockType::Paragraph;
    int level = 0;

    switch (type) {
        case MD_BLOCK_H: {
            auto* headingDetail = static_cast<MD_BLOCK_H_DETAIL*>(detail);
            blockType = BlockType::Heading;
            level = headingDetail->level;
            break;
        }
        case MD_BLOCK_CODE:
            blockType = BlockType::CodeBlock;
            break;
        case MD_BLOCK_QUOTE:
            blockType = BlockType::Quote;
            break;
        case MD_BLOCK_LI:
            blockType = BlockType::ListItem;
            break;
        case MD_BLOCK_TABLE:
            blockType = BlockType::Table;
            break;
        case MD_BLOCK_HTML:
            blockType = BlockType::Html;
            break;
        default:
            blockType = BlockType::Paragraph;
            break;
    }

    ctx->document->blocks.push_back(Block{blockType, level, ""});
    return 0;
}

int leaveBlock(MD_BLOCKTYPE, void*, void*) {
    return 0;
}

int enterSpan(MD_SPANTYPE, void*, void*) {
    return 0;
}

int leaveSpan(MD_SPANTYPE, void*, void*) {
    return 0;
}

int textCallback(MD_TEXTTYPE, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    auto* ctx = static_cast<ParseContext*>(userdata);
    if (!ctx->document->blocks.empty()) {
        ctx->document->blocks.back().text.append(text, size);
    }
    return 0;
}

} // namespace

Document MarkdownParser::parse(const std::string& markdownSource) const {
    Document document;
    ParseContext context{&document};

    MD_PARSER parser{};
    parser.abi_version = 0;
    parser.flags = MD_DIALECT_GITHUB;
    parser.enter_block = enterBlock;
    parser.leave_block = leaveBlock;
    parser.enter_span = enterSpan;
    parser.leave_span = leaveSpan;
    parser.text = textCallback;
    parser.debug_log = nullptr;
    parser.syntax = nullptr;

    md_parse(markdownSource.c_str(), static_cast<MD_SIZE>(markdownSource.size()), &parser,
             &context);

    return document;
}

} // namespace noctis::md
