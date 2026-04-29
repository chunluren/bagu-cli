#include "db/migrations.h"

namespace bagu::db {

namespace {

// ============================================================================
// 0001 — 初始 schema：topic / chapter / card / review / review_history + FTS5
// ============================================================================
constexpr const char* kMigration0001 = R"SQL(
CREATE TABLE topic (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT NOT NULL UNIQUE,
    title       TEXT NOT NULL,
    file_path   TEXT NOT NULL,
    file_hash   TEXT NOT NULL,
    imported_at INTEGER NOT NULL,
    updated_at  INTEGER NOT NULL
);
CREATE INDEX idx_topic_name ON topic(name);

CREATE TABLE chapter (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    topic_id    INTEGER NOT NULL,
    name        TEXT NOT NULL,
    chapter_no  INTEGER NOT NULL,
    parent_id   INTEGER,
    FOREIGN KEY (topic_id) REFERENCES topic(id) ON DELETE CASCADE,
    FOREIGN KEY (parent_id) REFERENCES chapter(id) ON DELETE CASCADE
);
CREATE INDEX idx_chapter_topic ON chapter(topic_id);

CREATE TABLE card (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    topic_id        INTEGER NOT NULL,
    chapter_id      INTEGER,
    question        TEXT NOT NULL,
    answer          TEXT NOT NULL,
    difficulty      INTEGER DEFAULT 2,
    tags            TEXT,
    source_line     INTEGER,
    card_type       TEXT DEFAULT 'qa',
    created_at      INTEGER NOT NULL,
    updated_at      INTEGER NOT NULL,
    FOREIGN KEY (topic_id) REFERENCES topic(id) ON DELETE CASCADE,
    FOREIGN KEY (chapter_id) REFERENCES chapter(id) ON DELETE SET NULL
);
CREATE INDEX idx_card_topic ON card(topic_id);
CREATE INDEX idx_card_chapter ON card(chapter_id);

CREATE TABLE review (
    card_id         INTEGER PRIMARY KEY,
    last_review     INTEGER,
    next_review     INTEGER,
    interval_days   INTEGER DEFAULT 0,
    ease_factor     REAL DEFAULT 2.5,
    repetitions     INTEGER DEFAULT 0,
    review_count    INTEGER DEFAULT 0,
    correct_count   INTEGER DEFAULT 0,
    suspended       INTEGER DEFAULT 0,
    FOREIGN KEY (card_id) REFERENCES card(id) ON DELETE CASCADE
);
CREATE INDEX idx_review_next ON review(next_review) WHERE suspended = 0;

CREATE TABLE review_history (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    card_id     INTEGER NOT NULL,
    reviewed_at INTEGER NOT NULL,
    score       INTEGER NOT NULL,
    duration_ms INTEGER,
    FOREIGN KEY (card_id) REFERENCES card(id) ON DELETE CASCADE
);
CREATE INDEX idx_history_card ON review_history(card_id);
CREATE INDEX idx_history_time ON review_history(reviewed_at);
)SQL";

// ============================================================================
// 0002 — FTS5 全文索引
// ============================================================================
constexpr const char* kMigration0002 = R"SQL(
CREATE VIRTUAL TABLE card_fts USING fts5(
    question,
    answer,
    tags,
    content='card',
    content_rowid='id',
    tokenize='unicode61 remove_diacritics 1'
);

CREATE TRIGGER card_fts_insert AFTER INSERT ON card BEGIN
    INSERT INTO card_fts(rowid, question, answer, tags)
    VALUES (new.id, new.question, new.answer, COALESCE(new.tags, ''));
END;

CREATE TRIGGER card_fts_delete AFTER DELETE ON card BEGIN
    DELETE FROM card_fts WHERE rowid = old.id;
END;

CREATE TRIGGER card_fts_update AFTER UPDATE ON card BEGIN
    UPDATE card_fts
    SET question = new.question,
        answer = new.answer,
        tags = COALESCE(new.tags, '')
    WHERE rowid = new.id;
END;
)SQL";

// ============================================================================
// 0003 — interview 会话表
// ============================================================================
constexpr const char* kMigration0003 = R"SQL(
CREATE TABLE interview_session (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    topic           TEXT NOT NULL,
    started_at      INTEGER NOT NULL,
    ended_at        INTEGER,
    total_score     REAL,
    question_count  INTEGER DEFAULT 0,
    llm_provider    TEXT,
    llm_model       TEXT
);

CREATE TABLE interview_qa (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id      INTEGER NOT NULL,
    question_no     INTEGER NOT NULL,
    question        TEXT NOT NULL,
    user_answer     TEXT,
    ai_score        INTEGER,
    ai_feedback     TEXT,
    duration_ms     INTEGER,
    FOREIGN KEY (session_id) REFERENCES interview_session(id) ON DELETE CASCADE
);
CREATE INDEX idx_qa_session ON interview_qa(session_id);
)SQL";

}  // namespace

void register_all_migrations(Database& db) {
    db.register_migration(1, "initial schema (topic/chapter/card/review)",
        kMigration0001);
    db.register_migration(2, "card_fts virtual table + sync triggers",
        kMigration0002);
    db.register_migration(3, "interview_session + interview_qa",
        kMigration0003);
}

}  // namespace bagu::db
