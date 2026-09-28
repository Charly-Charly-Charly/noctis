#include <gtest/gtest.h>

#include "Filesystem/Frontmatter.h"

using namespace noctis::fs;

TEST(FrontmatterTest, DocumentWithoutFrontmatterIsAllBody) {
    Document document = splitDocument("# Clase 5\n\nContenido\n");

    EXPECT_TRUE(document.frontmatter.empty());
    EXPECT_EQ(document.body, "# Clase 5\n\nContenido\n");
}

TEST(FrontmatterTest, SplitsFrontmatterFromBody) {
    Document document = splitDocument("---\nid: abc-123\n---\n# Clase 5\n");

    EXPECT_EQ(document.frontmatter, "id: abc-123\n");
    EXPECT_EQ(document.body, "# Clase 5\n");
}

TEST(FrontmatterTest, UnclosedBlockIsNotFrontmatter) {
    const std::string raw = "---\nid: abc\n# Clase 5\n";
    Document document = splitDocument(raw);

    EXPECT_TRUE(document.frontmatter.empty());
    EXPECT_EQ(document.body, raw);
}

TEST(FrontmatterTest, RoundTripPreservesDocument) {
    const std::string raw = "---\nid: abc-123\ntags: derecho\n---\n# Clase 5\n\nTexto\n";

    EXPECT_EQ(joinDocument(splitDocument(raw)), raw);
}

TEST(FrontmatterTest, ReadsField) {
    EXPECT_EQ(frontmatterField("id: abc-123\ntags: derecho\n", "id"), "abc-123");
    EXPECT_EQ(frontmatterField("id: abc-123\n", "tags"), std::nullopt);
}

TEST(FrontmatterTest, UpsertAddsFieldWhenMissing) {
    EXPECT_EQ(upsertFrontmatterField("", "id", "abc"), "id: abc\n");
    EXPECT_EQ(upsertFrontmatterField("tags: derecho\n", "id", "abc"),
              "tags: derecho\nid: abc\n");
}

TEST(FrontmatterTest, UpsertReplacesExistingFieldAndKeepsOthers) {
    EXPECT_EQ(upsertFrontmatterField("id: viejo\ntags: derecho\n", "id", "nuevo"),
              "id: nuevo\ntags: derecho\n");
}
