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

#include <string>
#include <vector>
#include <glib.h>
#include "PYEngEditor.h"

static const char * SQL_CREATE_DB =
    "CREATE TABLE IF NOT EXISTS english ("
    "word TEXT NOT NULL,"
    "freq FLOAT NOT NULL DEFAULT(0)"
    ");";

static const char * SQL_DB_LIST = 
    "SELECT word FROM english WHERE word LIKE \"%s%\" ORDER BY freq DESC;";

static const char * SQL_DB_SELECT = 
    "SELECT word, freq FROM english WHERE word = \"%s\";";

static const char * SQL_DB_UPDATE =
    "UPDATE english SET freq = \"%f\" WHERE word = \"%s\";";

static const char * SQL_DB_INSERT =
    "INSERT INTO english (word, freq) VALUES (\"%s\", \"%f\");";

class EnglishEditor{
private:
    sqlite3 * m_system_db;
    sqlite3 * m_user_db;
    std::string m_sql;

public:
    EnglishEditor(){
        m_system_sql = NULL;
        m_user_sql = NULL;
        m_sql = "";
    }

    bool isDatabaseExisted(const char * filename) {
         gboolean result = g_file_test(filename, G_FILE_TEST_IS_REGULAR);
         sqlite3 * tmp_db = NULL;
         if ( sqlite3_open_v2 ( filename, &tmp_db,
                                SQLITE_OPEN_READONLY, NULL) != SQLITE_OK )
             return false;
         /* TODO: Check the desc table */
         m_sql = "SELECT value FROM desc WHERE 'name' = 'version';";
         

         return true;
    }

    bool createDatabase(const char * filename) {
        sqlite3 * tmp_db = NULL;
        if ( sqlite3_open_v2 ( filename, &tmp_db,
                               SQLITE_OPEN_READWRITE | 
                               SQLITE_OPEN_CREATE, NULL) != SQLITE_OK )
            return false;
        /* Create DESCription table */
        m_sql = "BEGIN TRANSACTION;\n";
        m_sql << "CREATE TABLE IF NOT EXISTS desc (name TEXT PRIMARY KEY, value TEXT);\n";
        m_sql << "INSERT OR IGNORE INTO desc VALUES ('version', '1.2.0');"
        m_sql << "COMMIT;\n";

        char * errmsg = NULL;
        int result = sqlite3_exec(tmp_db, m_sql.c_str(), NULL, NULL, &errmsg);
        if ( result ) {
            fprintf(STDERR, "%s\n", errmsg);
            sqlite3_close(tmp_db);
            return false;
        }

        m_sql = "";
        /* Create Schema */
        result = sqlite3_exec(tmp_db, SQL_CREATE_DB, NULL, NULL, &errmsg);
        if ( result ) {
            fprintf(STDERR, "%s\n", errmsg);
            sqlite3_close(tmp_db);
            return false;
        }
        return true;
    }

    bool openDatabase(const char * system_db, const char * user_db){
        
    }

    /* List the words in freq order. */
    bool listWords(const char * prefix, std::vector<std::string> & words);
    /* Get the sum of the freq of user and system sqlite db */
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
