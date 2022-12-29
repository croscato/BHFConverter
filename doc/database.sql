-- SPDX-License-Identifier: MIT
-- Copyright (c) 2022 Gustavo Ribeiro Croscato

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS tbl_context (
      context_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT
    , context_offset INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_context_offset ON tbl_context (context_offset);

CREATE TABLE IF NOT EXISTS tbl_index (
      index_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT
    , context_id INTEGER NOT NULL
    , index_value VARCHAR NOT NULL

    , CONSTRAINT fk_index_context FOREIGN KEY (context_id) REFERENCES tbl_context (context_id)
);

CREATE TABLE IF NOT EXISTS tbl_text (
      context_id INTEGER NOT NULL
    , text_value BLOB NOT NULL

    , CONSTRAINT pk_text PRIMARY KEY (context_id)
    , CONSTRAINT fk_text_context FOREIGN KEY (context_id) REFERENCES tbl_context (context_id)
);

CREATE TABLE IF NOT EXISTS tbl_keyword (
      context_id INTEGER NOT NULL
    , keyword_up_context INTEGER
    , keyword_down_context INTEGER

    , CONSTRAINT pk_keyword PRIMARY KEY (context_id)
    , CONSTRAINT fk_keyword_text FOREIGN KEY (context_id) REFERENCES tbl_text (context_id)
    , CONSTRAINT fk_keyword_up_context FOREIGN KEY (keyword_up_context) REFERENCES tbl_context (context_id)
    , CONSTRAINT fk_keyword_down_context FOREIGN KEY (keyword_down_context) REFERENCES tbl_context (context_id)
);

CREATE TABLE IF NOT EXISTS tbl_keyword_list (
      context_id INTEGER NOT NULL
    , keyword_index INTEGER NOT NULL
    , keyword_context INTEGER NOT NULL

    , CONSTRAINT pk_keyword_list PRIMARY KEY (context_id, keyword_index)
    , CONSTRAINT fk_keyword_list_keyword FOREIGN KEY (context_id) REFERENCES tbl_keyword (context_id)
    , CONSTRAINT fk_keyword_list_context FOREIGN KEY (keyword_context) REFERENCES tbl_context (context_id)
);
