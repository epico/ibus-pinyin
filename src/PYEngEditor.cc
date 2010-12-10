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
