/* vim:set et ts=4 sts=4:
 *
 * ibus-pinyin - The Chinese PinYin engine for IBus
 *
 * Copyright (c) 2010 Peng Wu <alexepico@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <assert.h>
#include <string.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <sqlite3.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "PYString.h"
#include "PYLookupTable.h"
#include "PYEngEditor.h"

static const char * SQL_CREATE_DB =
    "CREATE TABLE IF NOT EXISTS english ("
    "word TEXT NOT NULL PRIMARY KEY,"
    "freq FLOAT NOT NULL DEFAULT(0)"
    ");";

static const char * SQL_ATTACH_DB =
    "ATTACH DATABASE '%s' AS user;";

static const char * SQL_DB_LIST = 
    "SELECT word FROM ( SELECT * FROM english UNION ALL SELECT * FROM user.english) WHERE word LIKE \"%s%%\" GROUP BY word ORDER BY SUM(freq) DESC;";

static const char * SQL_DB_SELECT = 
    "SELECT word, freq FROM user.english WHERE word = \"%s\";";

static const char * SQL_DB_UPDATE =
    "UPDATE user.english SET freq = \"%f\" WHERE word = \"%s\";";

static const char * SQL_DB_INSERT =
    "INSERT INTO user.english (word, freq) VALUES (\"%s\", \"%f\");";

namespace PY {

class EnglishDatabase{
private:
    sqlite3 * m_sqlite;
    String m_sql;

    bool executeSQL (){
        assert(m_sqlite != NULL);
        gchar * errmsg = NULL;
        if ( sqlite3_exec (m_sqlite, m_sql.c_str(), NULL, NULL, &errmsg)
             != SQLITE_OK) {
            g_warning ("%s: %s", errmsg, m_sql.c_str());
            sqlite3_free (errmsg);
            return false;
        }
        return true;
    }
public:
    EnglishDatabase(){
        m_sqlite = NULL;
        m_sql = "";
    }

    bool isDatabaseExisted(const char * filename) {
         gboolean result = g_file_test(filename, G_FILE_TEST_IS_REGULAR);
         if (!result)
             return false;

         sqlite3 * tmp_db = NULL;
         if ( sqlite3_open_v2 ( filename, &tmp_db,
                                SQLITE_OPEN_READONLY, NULL) != SQLITE_OK ){
             sqlite3_close(tmp_db);
             return false;
         }

         /* TODO: Check the desc table */
         sqlite3_stmt * stmt = NULL;
         const char * tail = NULL;
         m_sql = "SELECT value FROM desc WHERE 'name' = 'version';";
         result = sqlite3_prepare_v2( tmp_db, m_sql.c_str(), -1, &stmt, &tail);
         assert(result == SQLITE_OK);
         result = sqlite3_step(stmt);
         if ( result != SQLITE_ROW)
             return false;
         result = sqlite3_column_type (stmt, 0);
         if ( result != SQLITE_TEXT)
             return false;
         const char * version = (const char *) sqlite3_column_text(stmt, 0);
         result = sqlite3_finalize(stmt);
         assert ( result == SQLITE_OK);
         if ( strcmp("1.2.0", version ) != 0)
             return false;
         sqlite3_close(tmp_db);
         return true;
    }

    bool createDatabase(const char * filename) {
        /* unlink the old database. */
        gboolean retval = g_file_test(filename, G_FILE_TEST_IS_REGULAR);
        if ( retval ) {
            int result = g_unlink(filename);
            if ( result == -1 )
                return false;
        }

        sqlite3 * tmp_db = NULL;
        if ( sqlite3_open_v2 ( filename, &tmp_db,
                               SQLITE_OPEN_READWRITE | 
                               SQLITE_OPEN_CREATE, NULL) != SQLITE_OK ) {
            sqlite3_close(tmp_db);
            return false;
        }

        /* Create DESCription table */
        m_sql = "BEGIN TRANSACTION;\n";
        m_sql << "CREATE TABLE IF NOT EXISTS desc (name TEXT PRIMARY KEY, value TEXT);\n";
        m_sql << "INSERT OR IGNORE INTO desc VALUES ('version', '1.2.0');";
        m_sql << "COMMIT;\n";

        if ( !executeSQL() ) {
            sqlite3_close(tmp_db);
            return false;
        }

        /* Create Schema */
        m_sql = SQL_CREATE_DB;
        if ( !executeSQL() ) {
            sqlite3_close(tmp_db);
            return false;
        }
        m_sql = "";
        return true;
    }

    bool openDatabase(const char * system_db, const char * user_db){
        if ( !isDatabaseExisted(system_db) )
            return false;
        if ( !isDatabaseExisted(user_db)) {
            bool result = createDatabase(user_db);
            if ( !result )
                return false;
        }
        /* do database attach here. :) */
        if ( sqlite3_open_v2( system_db, &m_sqlite,
                              SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK ) {
            sqlite3_close(m_sqlite);
            m_sqlite = NULL;                
            return false;
        }

        char * errmsg = NULL;
        m_sql = "";
        m_sql.printf(SQL_ATTACH_DB, user_db);
        if ( !executeSQL() ) {
            sqlite3_close(m_sqlite);
            return false;
        }
        return true;
    }

    /* List the words in freq order. */
    bool listWords(const char * prefix, std::vector<std::string> & words){
        
    }

    /* Get the freq of user sqlite db. */
    bool getWordInfo(const char * word, float & freq);
    /* Update the freq with delta value. */
    bool updateWord(const char * word, float delta);
    /* Insert the word into user db with the initial freq. */
    bool insertWord(const char * word, float initial_freq);
};


/* Auxiliary Functions */

void
EnglishEditor::pageUp (void)
{
    if ( G_LIKELY(m_lookup_table.pageUp ())) {
        update();
    }
}

void
EnglishEditor::pageDown (void)
{
    if ( G_LIKELY(m_lookup_table.pageDown ())) {
        update();
    }
}

void
EnglishEditor::cursorUp (void)
{
    if ( G_LIKELY(m_lookup_table.cursorUp ())) {
        update();
    }
}

void
EnglishEditor::cursorDown (void)
{
    if ( G_LIKELY(m_lookup_table.cursorDown())) {
        update();
    }
}

void
EnglishEditor::update (void)
{
    updateLookupTable ();
    updatePreeditText ();
    updateAuxiliaryText ();
}

void
EnglishEditor::updateLookupTable (void)
{
    if (m_lookup_table.size ()) {
        Editor::updateLookupTableFast (m_lookup_table, TRUE);
    }
    else {
        hideLookupTable ();
    }
}

void
EnglishEditor::updatePreeditText (void)
{
    if ( G_UNLIKELY(m_preedit_text.empty ()) ) {
        hidePreeditText ();
        return;
    }

    StaticText preedit_text (m_preedit_text);
    Editor::updatePreeditText (preedit_text, m_cursor, TRUE);
}

void
EnglishEditor::updateAuxiliaryText (void)
{
    if ( G_UNLIKELY(m_auxiliary_text.empty ()) ) {
        hideAuxiliaryText ();
        return;
    }
    
    StaticText aux_text (m_auxiliary_text);
    Editor::updateAuxiliaryText (aux_text, TRUE);
}

};
