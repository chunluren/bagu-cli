#include "parser/markdown_parser.h"

#include <gtest/gtest.h>

namespace bagu::parser {

class MarkdownParserTest : public ::testing::Test {
protected:
    MarkdownParser parser;
};

TEST_F(MarkdownParserTest, ParseEmpty_ReturnsEmptyDoc) {
    auto r = parser.parse_string("", "test.md");
    ASSERT_TRUE(r.is_ok());
    EXPECT_TRUE(r.value().chapters.empty());
    EXPECT_TRUE(r.value().cards.empty());
}

TEST_F(MarkdownParserTest, ParseTitle_ExtractsFromH1) {
    auto r = parser.parse_string("# MySQL 八股文档\n\n正文", "x.md");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().title, "MySQL 八股文档");
}

TEST_F(MarkdownParserTest, ParseChapter_ExtractsChapterNumber) {
    auto r = parser.parse_string(
        "# Title\n"
        "## 第 3 章 索引\n"
        "正文\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    ASSERT_EQ(r.value().chapters.size(), 1u);
    auto& ch = r.value().chapters[0];
    EXPECT_EQ(ch.chapter_no, 3);
    EXPECT_EQ(ch.level, 2);
    EXPECT_EQ(ch.name, "索引");
}

TEST_F(MarkdownParserTest, ParseSection_CreatesChapterAndSectionCard) {
    auto r = parser.parse_string(
        "## 第 3 章 索引\n"
        "### 3.1 B+ 树\n"
        "B+ 树是一种平衡多叉树。\n"
        "叶子节点链表相连。\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    auto& doc = r.value();
    EXPECT_EQ(doc.chapters.size(), 2u);   // 1 个 ## + 1 个 ###
    ASSERT_EQ(doc.cards.size(), 1u);
    EXPECT_EQ(doc.cards[0].card_type, "section");
    EXPECT_EQ(doc.cards[0].question, "B+ 树");
    EXPECT_EQ(doc.cards[0].chapter_no, 3);
    EXPECT_NE(doc.cards[0].answer.find("B+ 树是一种平衡多叉树"), std::string::npos);
}

TEST_F(MarkdownParserTest, ParseQA_SingleQuestion) {
    auto r = parser.parse_string(
        "## 第 11 章 题库\n"
        "**Q1. 什么是 MVCC？**\n"
        "MVCC 是多版本并发控制\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    auto& doc = r.value();
    ASSERT_EQ(doc.cards.size(), 1u);
    EXPECT_EQ(doc.cards[0].card_type, "qa");
    EXPECT_EQ(doc.cards[0].question, "什么是 MVCC？");
    EXPECT_EQ(doc.cards[0].chapter_no, 11);
    EXPECT_NE(doc.cards[0].answer.find("MVCC 是多版本"), std::string::npos);
}

TEST_F(MarkdownParserTest, ParseQA_AnswerOnSameLine) {
    auto r = parser.parse_string(
        "## 第 11 章 题库\n"
        "**Q1. ACID 是什么？** 原子性 一致性 隔离性 持久性\n"
        "**Q2. 锁有几种？** 行锁 表锁\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    auto& doc = r.value();
    ASSERT_EQ(doc.cards.size(), 2u);
    EXPECT_EQ(doc.cards[0].question, "ACID 是什么？");
    EXPECT_NE(doc.cards[0].answer.find("原子性"), std::string::npos);
    EXPECT_EQ(doc.cards[1].question, "锁有几种？");
}

TEST_F(MarkdownParserTest, ParseQA_MultiQuestionsConsecutive) {
    auto r = parser.parse_string(
        "## 第 11 章\n"
        "**Q1. 第一题** 答1\n"
        "**Q2. 第二题** 答2\n"
        "**Q3. 第三题** 答3\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().cards.size(), 3u);
}

TEST_F(MarkdownParserTest, CodeBlock_NotMisparsed) {
    auto r = parser.parse_string(
        "## 第 1 章\n"
        "### 1.1 示例\n"
        "看代码：\n"
        "```cpp\n"
        "## fake heading inside code\n"
        "**Q99. fake question** answer\n"
        "```\n"
        "正文继续\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    auto& doc = r.value();
    // 只有 ## 第 1 章 + ### 1.1 = 2 个章节
    EXPECT_EQ(doc.chapters.size(), 2u);
    // 1 张 section card（### 1.1），不应有 qa
    ASSERT_EQ(doc.cards.size(), 1u);
    EXPECT_EQ(doc.cards[0].card_type, "section");
}

TEST_F(MarkdownParserTest, MixedQAAndSection) {
    auto r = parser.parse_string(
        "## 第 1 章 基础\n"
        "### 1.1 概念\n"
        "这是基础概念。\n"
        "## 第 11 章 题库\n"
        "**Q1. 题1** 答1\n"
        "**Q2. 题2** 答2\n", "x.md");
    ASSERT_TRUE(r.is_ok());
    auto& doc = r.value();
    EXPECT_EQ(doc.cards.size(), 3u);  // 1 section + 2 qa

    int section_count = 0, qa_count = 0;
    for (const auto& c : doc.cards) {
        if (c.card_type == "section") section_count++;
        else if (c.card_type == "qa") qa_count++;
    }
    EXPECT_EQ(section_count, 1);
    EXPECT_EQ(qa_count, 2);
}

TEST(TopicNameFromFilename, BasicCases) {
    EXPECT_EQ(topic_name_from_filename("mysql-interview.md"), "mysql");
    EXPECT_EQ(topic_name_from_filename("redis-interview.md"), "redis");
    EXPECT_EQ(topic_name_from_filename("cpp-network-interview.md"), "cpp-network");
    EXPECT_EQ(topic_name_from_filename("MySQL.md"), "mysql");
}

TEST(TopicNameFromFilename, ChineseSuffix) {
    EXPECT_EQ(topic_name_from_filename("redis 八股文档.md"), "redis");
    EXPECT_EQ(topic_name_from_filename("Java 八股.md"), "java");
}

TEST(TopicNameFromFilename, EmptyOrEdge) {
    EXPECT_EQ(topic_name_from_filename("notes.md"), "notes");
    EXPECT_EQ(topic_name_from_filename(""), "untitled");
}

}  // namespace bagu::parser
