//---------------------------------------------------------------------------
// WordEngine.cpp
//
// A class to handle the loading and searching of words.
//
// Copyright 2004, 2005, 2006 Michael W Thelen <mike@pietdepsi.com>.
//
// This file is part of Zyzzyva.
//
// Zyzzyva is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Zyzzyva is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//---------------------------------------------------------------------------

#include "WordEngine.h"
#include "LetterBag.h"
#include "LoadDefinitionsThread.h"
#include "Auxil.h"
#include "Defs.h"
#include <QFile>
#include <QRegExp>
#include <set>

const int MAX_DEFINITION_LINKS = 3;

using namespace Defs;
using std::set;
using std::map;
using std::make_pair;
using std::multimap;


//---------------------------------------------------------------------------
//  importTextFile
//
//! Import words from a text file.  The file is assumed to be in plain text
//! format, containing one word per line.
//
//! @param filename the name of the file to import
//! @param lexName the name of the lexicon
//! @param loadDefinitions whether to load word definitions
//! @param errString returns the error string in case of error
//! @return the number of words imported
//---------------------------------------------------------------------------
int
WordEngine::importTextFile (const QString& filename, const QString& lexName,
                            bool loadDefinitions, QString* errString)
{
    QFile file (filename);
    if (!file.open (QIODevice::ReadOnly | QIODevice::Text)) {
        if (errString)
            *errString = "Can't open file '" + filename + "': "
            + file.errorString();
        return 0;
    }

    int imported = 0;
    char* buffer = new char [MAX_INPUT_LINE_LEN];
    while (file.readLine (buffer, MAX_INPUT_LINE_LEN) > 0) {
        QString line (buffer);
        line = line.simplified();
        if (!line.length() || (line.at (0) == '#'))
            continue;
        QString word = line.section (' ', 0, 0).toUpper();

        if (!graph.containsWord (word)) {
            QString alpha = Auxil::getAlphagram (word);
            ++numAnagramsMap[alpha];
        }

        graph.addWord (word);
        if (loadDefinitions) {
            QString definition = line.section (' ', 1);
            addDefinition (word, definition);
        }
        ++imported;
    }

    delete[] buffer;

    lexiconName = lexName;
    return imported;
}

//---------------------------------------------------------------------------
//  importDawgFile
//
//! Import words from a DAWG file as generated by Graham Toal's dawgutils
//! programs: http://www.gtoal.com/wordgames/dawgutils/
//
//! @param filename the name of the DAWG file to import
//! @param lexName the name of the lexicon
//! @param reverse whether the DAWG contains reversed words
//! @param errString returns the error string in case of error
//! @return true if successful, false otherwise
//---------------------------------------------------------------------------
bool
WordEngine::importDawgFile (const QString& filename, const QString& lexName,
                            bool reverse, QString* errString)
{
    bool ok = graph.importDawgFile (filename, reverse, errString);

    if (!ok)
        return false;

    if (reverse)
        return true;

    lexiconName = lexName;
    return true;
}

//---------------------------------------------------------------------------
//  importDefinitions
//
//! Import definitions from a text file.
//
//! @param filename the file to import definitions from
//---------------------------------------------------------------------------
void
WordEngine::importDefinitions (const QString& filename)
{
    definitionsThread->setFilename (filename);
    connect (definitionsThread, SIGNAL (loaded()),
             SLOT (definitionsLoaded()));
    definitionsThread->start();
}

//---------------------------------------------------------------------------
//  importNumWords
//
//! Import a word count from a text file.
//
//! @param filename the file to import the word count from
//! @return the word count
//---------------------------------------------------------------------------
int
WordEngine::importNumWords (const QString& filename)
{
    QFile file (filename);
    if (!file.open (QIODevice::ReadOnly | QIODevice::Text))
        return 0;

    char* buffer = new char [MAX_INPUT_LINE_LEN];
    if (file.readLine (buffer, MAX_INPUT_LINE_LEN) > 0) {
        QString line (buffer);
        return line.simplified().section (' ', 0, 0).toInt();
    }
    return 0;
}

//---------------------------------------------------------------------------
//  importNumAnagrams
//
//! Import anagram counts from a text file.
//
//! @param filename the file to import anagram counts definitions from
//---------------------------------------------------------------------------
void
WordEngine::importNumAnagrams (const QString& filename)
{
    QFile file (filename);
    if (!file.open (QIODevice::ReadOnly | QIODevice::Text))
        return;

    char* buffer = new char [MAX_INPUT_LINE_LEN];
    while (file.readLine (buffer, MAX_INPUT_LINE_LEN) > 0) {
        QString line (buffer);
        line = line.simplified();
        if (!line.length() || (line.at (0) == '#'))
            continue;

        QString alphagram = line.section (' ', 0, 0).toUpper();
        int count = line.section (' ', 1).toInt();
        numAnagramsMap[alphagram] = count;
    }
}

//---------------------------------------------------------------------------
//  importStems
//
//! Import stems from a file.  The file is assumed to be in plain text format,
//! containing one stem per line.  The file is also assumed to contain stems
//! of equal length.  All stems of different length than the first stem will
//! be discarded.
//
//! @param filename the name of the file to import
//! @param errString returns the error string in case of error
//! @return the number of stems imported
//---------------------------------------------------------------------------
int
WordEngine::importStems (const QString& filename, QString* errString)
{
    QFile file (filename);
    if (!file.open (QIODevice::ReadOnly | QIODevice::Text)) {
        if (errString)
            *errString = "Can't open file '" + filename + "': "
            + file.errorString();
        return -1;
    }

    // XXX: At some point, may want to consider allowing words of varying
    // lengths to be in the same file?
    QStringList words;
    set<QString> alphagrams;
    int imported = 0;
    int length = 0;
    char* buffer = new char [MAX_INPUT_LINE_LEN];
    while (file.readLine (buffer, MAX_INPUT_LINE_LEN) > 0) {
        QString line (buffer);
        line = line.simplified();
        if (!line.length() || (line.at (0) == '#'))
            continue;
        QString word = line.section (' ', 0, 0);

        if (!length)
            length = word.length();

        if (length != int (word.length()))
            continue;

        words << word;
        alphagrams.insert (Auxil::getAlphagram (word));
        ++imported;
    }
    delete[] buffer;

    // Insert the stem list into the map, or append to an existing stem list
    map<int, QStringList>::iterator it = stems.find (length);
    if (it == stems.end()) {
        stems.insert (make_pair (length, words));
        stemAlphagrams.insert (make_pair (length, alphagrams));
    }
    else {
        it->second += words;
        std::map< int, set<QString> >::iterator it =
            stemAlphagrams.find (length);
        set<QString>::iterator sit;
        for (sit = alphagrams.begin(); sit != alphagrams.end(); ++sit)
            (it->second).insert (*sit);
    }

    return imported;
}

//---------------------------------------------------------------------------
//  getNewInOwl2String
//
//! Read all new OWL2 words into a string, separated by spaces.  XXX: Right
//! now this is hard-coded to load a certain file for a specific purpose.
//! This whole concept should be more flexible.
//---------------------------------------------------------------------------
QString
WordEngine::getNewInOwl2String() const
{
    QFile file (Auxil::getWordsDir() + "/north-american/owl2-new-words.txt");
    if (!file.open (QIODevice::ReadOnly | QIODevice::Text))
        return QString::null;

    QStringList words;
    char* buffer = new char [MAX_INPUT_LINE_LEN];
    while (file.readLine (buffer, MAX_INPUT_LINE_LEN) > 0) {
        QString line (buffer);
        line = line.simplified();
        if (!line.length() || (line.at (0) == '#'))
            continue;
        QString word = line.section (' ', 0, 0);
        words.append (word);
    }
    delete[] buffer;

    return words.join (" ");
}

//---------------------------------------------------------------------------
//  isAcceptable
//
//! Determine whether a word is acceptable.
//
//! @param word the word to look up
//! @return true if acceptable, false otherwise
//---------------------------------------------------------------------------
bool
WordEngine::isAcceptable (const QString& word) const
{
    return graph.containsWord (word);
}

//---------------------------------------------------------------------------
//  search
//
//! Search for acceptable words matching a search specification.
//
//! @param spec the search specification
//! @param allCaps whether to ensure the words in the list are all caps
//! @return a list of acceptable words
//---------------------------------------------------------------------------
QStringList
WordEngine::search (const SearchSpec& spec, bool allCaps) const
{
    SearchSpec optimizedSpec = spec;
    optimizedSpec.optimize();

    // Big optimization if the only conditions are conditions that can be
    // matched without searching the word graph.  Also, replace Must Belong To
    // New in OWL2 conditions with Must Be in Word List conditions.
    bool mustSearchGraph = false;
    bool wordListCondition = false;
    int probRangeMin = 0;
    int probRangeMax = 0;
    int i = 0;
    QListIterator<SearchCondition> cit (optimizedSpec.conditions);
    while (cit.hasNext()) {
        SearchCondition condition = cit.next();
        switch (condition.type) {
            case SearchCondition::InWordList:
            wordListCondition = true;
            if (condition.negated)
                mustSearchGraph = true;
            break;

            case SearchCondition::NumAnagrams:
            break;

            case SearchCondition::BelongToGroup: {
                SearchSet searchSet = Auxil::stringToSearchSet
                    (condition.stringValue);
                if (searchSet == SetNewInOwl2) {
                    condition.type = SearchCondition::InWordList;
                    condition.stringValue = getNewInOwl2String();
                    optimizedSpec.conditions.replace (i, condition);
                    wordListCondition = true;
                }
                else {
                    mustSearchGraph = true;
                }
            }
            break;

            case SearchCondition::ProbabilityOrder:
            probRangeMin = condition.minValue;
            probRangeMax = condition.maxValue;
            break;

            default:
            mustSearchGraph = true;
            break;
        }
        ++i;
    }
    if (wordListCondition && !mustSearchGraph)
        return nonGraphSearch (optimizedSpec);

    QStringList wordList = graph.search (optimizedSpec);

    // Check special postconditions
    QStringList::iterator wit;
    for (wit = wordList.begin(); wit != wordList.end();) {
        if (matchesConditions (*wit, optimizedSpec.conditions))
            ++wit;
        else
            wit = wordList.erase (wit);
    }

    // Keep only words in the probability order range
    if (probRangeMax > 0) {

        if (probRangeMin > wordList.size()) {
            wordList.clear();
            return wordList;
        }

        if (probRangeMin <= 0)
            probRangeMin = 1;

        LetterBag bag;
        QMap<QString, QString> probMap;

        QString word;
        foreach (word, wordList) {
            QString radix;
            radix.sprintf ("%09.0f",
                1e9 - 1 - bag.getNumCombinations (word.toUpper()));
            radix += word.toUpper();
            probMap.insert (radix, word);
        }

        wordList = probMap.values().mid (probRangeMin - 1,
                                         probRangeMax - probRangeMin + 1);
    }

    // Convert to all caps if necessary
    if (allCaps) {
        QStringList::iterator it;
        for (it = wordList.begin(); it != wordList.end(); ++it)
            *it = (*it).toUpper();
    }

    return wordList;
}

//---------------------------------------------------------------------------
//  alphagrams
//
//! Transform a list of strings into a list of alphagrams of those strings.
//! The created list may be shorter than the original list.
//
//! @param list the list of strings
//! @return a list of alphagrams
//---------------------------------------------------------------------------
QStringList
WordEngine::alphagrams (const QStringList& list) const
{
    QStringList alphagrams;
    QStringList::const_iterator it;

    // Insert into a set first, to remove duplicates
    set<QString> seen;
    for (it = list.begin(); it != list.end(); ++it) {
        seen.insert (Auxil::getAlphagram (*it));
    }

    set<QString>::iterator sit;
    for (sit = seen.begin(); sit != seen.end(); ++sit) {
        alphagrams << *sit;
    }
    return alphagrams;
}

//---------------------------------------------------------------------------
//  getDefinition
//
//! Return the definition associated with a word.
//
//! @param word the word whose definition to look up
//! @return the definition, or QString::null if no definition
//---------------------------------------------------------------------------
QString
WordEngine::getDefinition (const QString& word) const
{
    map<QString, multimap<QString, QString> >::const_iterator it =
        definitions.find (word);
    if (it == definitions.end())
        return QString::null;

    const multimap<QString, QString>& mmap = it->second;
    multimap<QString, QString>::const_iterator mit;
    QString definition;
    for (mit = mmap.begin(); mit != mmap.end(); ++mit) {
        if (!definition.isEmpty())
            definition += " / ";
        definition += replaceDefinitionLinks (mit->second,
                                              MAX_DEFINITION_LINKS);
    }
    return definition;
}

//---------------------------------------------------------------------------
//  getFrontHookLetters
//
//! Get a string of letters that can be added to the front of a word to make
//! other valid words.
//
//! @param word the word, assumed to be upper case
//! @return a string containing lower case letters representing front hooks
//---------------------------------------------------------------------------
QString
WordEngine::getFrontHookLetters (const QString& word) const
{
    SearchSpec spec;
    SearchCondition condition;
    condition.type = SearchCondition::PatternMatch;
    condition.stringValue = "?" + word;
    spec.conditions.append (condition);

    // Put first letter of each word in a set, for alphabetical order
    QStringList words = search (spec, true);
    set<QChar> letters;
    QStringList::iterator it;
    for (it = words.begin(); it != words.end(); ++it)
        letters.insert ((*it).at (0).toLower());

    QString ret;
    set<QChar>::iterator sit;
    for (sit = letters.begin(); sit != letters.end(); ++sit)
        ret += *sit;
    return ret;
}

//---------------------------------------------------------------------------
//  getBackHookLetters
//
//! Get a string of letters that can be added to the back of a word to make
//! other valid words.
//
//! @param word the word, assumed to be upper case
//! @return a string containing lower case letters representing back hooks
//---------------------------------------------------------------------------
QString
WordEngine::getBackHookLetters (const QString& word) const
{
    SearchSpec spec;
    SearchCondition condition;
    condition.type = SearchCondition::PatternMatch;
    condition.stringValue = word + "?";
    spec.conditions.append (condition);

    // Put first letter of each word in a set, for alphabetical order
    QStringList words = search (spec, true);
    set<QChar> letters;
    QStringList::iterator it;
    for (it = words.begin(); it != words.end(); ++it)
        letters.insert ((*it).at ((*it).length() - 1).toLower());

    QString ret;
    set<QChar>::iterator sit;
    for (sit = letters.begin(); sit != letters.end(); ++sit)
        ret += *sit;
    return ret;
}

//---------------------------------------------------------------------------
//  definitionsLoaded
//
//! Called when the definition-loading thread is complete.
//---------------------------------------------------------------------------
void
WordEngine::definitionsLoaded()
{
    definitions = definitionsThread->getDefinitions();
    definitionsThread->quit();
    definitionsThread->disconnect();
}

//---------------------------------------------------------------------------
//  anagramsLoaded
//
//! Called when the anagram-loading thread is complete.
//---------------------------------------------------------------------------
void
WordEngine::anagramsLoaded()
{
    //numAnagramsMap = a;
}

//---------------------------------------------------------------------------
//  matchesConditions
//
//! Test whether a word matches certain conditions.  Not all conditions in the
//! list are tested.  Only the conditions that cannot be easily tested in
//! WordGraph::search are tested here.
//
//! @param word the word to be tested
//! @param conditions the list of conditions to test
//! @return true if the word matches all special conditions, false otherwise
//---------------------------------------------------------------------------
bool
WordEngine::matchesConditions (const QString& word, const
                               QList<SearchCondition>& conditions) const
{
    QString wordUpper = word.toUpper();
    QListIterator<SearchCondition> it (conditions);
    while (it.hasNext()) {
        const SearchCondition& condition = it.next();

        switch (condition.type) {

            case SearchCondition::Prefix:
            if ((!isAcceptable (condition.stringValue + wordUpper))
                ^ condition.negated)
                return false;
            break;

            case SearchCondition::Suffix:
            if ((!isAcceptable (wordUpper + condition.stringValue))
                ^ condition.negated)
                return false;
            break;

            case SearchCondition::BelongToGroup: {
                SearchSet searchSet = Auxil::stringToSearchSet
                    (condition.stringValue);
                if (searchSet == UnknownSearchSet)
                    continue;
                if (!isSetMember (wordUpper, searchSet) ^ condition.negated)
                    return false;
            }
            break;

            case SearchCondition::InWordList:
            if ((!condition.stringValue.contains
                (QRegExp ("\\b" + wordUpper + "\\b")))
                ^ condition.negated)
                return false;
            break;

            case SearchCondition::NumAnagrams: {
                int num = numAnagrams (wordUpper); 
                if ((num < condition.minValue) || (num > condition.maxValue))
                    return false;
            }
            break;

            default: break;
        }
    }

    return true;
}

//---------------------------------------------------------------------------
//  isSetMember
//
//! Determine whether a word is a member of a set.  Assumes the word has
//! already been determined to be acceptable.
//
//! @param word the word to look up
//! @param ss the search set
//! @return true if a member of the set, false otherwise
//---------------------------------------------------------------------------
bool
WordEngine::isSetMember (const QString& word, SearchSet ss) const
{
    switch (ss) {
        case SetHookWords:
        return (isAcceptable (word.left (word.length() - 1)) ||
                isAcceptable (word.right (word.length() - 1)));

        case SetFrontHooks:
        return isAcceptable (word.right (word.length() - 1));

        case SetBackHooks:
        return isAcceptable (word.left (word.length() - 1));

        case SetTypeOneSevens: {
            if (word.length() != 7)
                return false;

            std::map< int, set<QString> >::const_iterator it =
                stemAlphagrams.find (word.length() - 1);
            if (it == stemAlphagrams.end())
                return false;

            const set<QString>& alphaset = it->second;
            QString agram = Auxil::getAlphagram (word);
            set<QString>::const_iterator ait;
            for (int i = 0; i < int (agram.length()); ++i) {
                ait = alphaset.find (agram.left (i) +
                                     agram.right (agram.length() - i - 1));
                if (ait != alphaset.end())
                    return true;
            }
            return false;
        }

        case SetTypeOneEights: {
            if (word.length() != 8)
                return false;

            std::map< int, set<QString> >::const_iterator it =
                stemAlphagrams.find (word.length() - 2);
            if (it == stemAlphagrams.end())
                return false;

            QString agram = Auxil::getAlphagram (word);

            // Compare the letters of the word with the letters of each
            // alphagram, ensuring that no more than two letters in the word
            // are missing from the alphagram.
            const set<QString>& alphaset = it->second;
            set<QString>::const_iterator ait;
            for (ait = alphaset.begin(); ait != alphaset.end(); ++ait) {
                QString setAlphagram = *ait;
                int missing = 0;
                int saIndex = 0;
                for (int i = 0; (i < int (agram.length())) &&
                                (saIndex < setAlphagram.length()); ++i)
                {
                    if (agram.at (i) == setAlphagram.at (saIndex))
                        ++saIndex;
                    else
                        ++missing;
                    if (missing > 2)
                        break;
                }
                if (missing <= 2)
                    return true;
            }
            return false;
        }

        case SetEightsFromSevenLetterStems: {
            if (word.length() != 8)
                return false;

            std::map< int, set<QString> >::const_iterator it =
                stemAlphagrams.find (word.length() - 1);
            if (it == stemAlphagrams.end())
                return false;

            const set<QString>& alphaset = it->second;
            QString agram = Auxil::getAlphagram (word);
            set<QString>::const_iterator ait;
            for (int i = 0; i < int (agram.length()); ++i) {
                ait = alphaset.find (agram.left (i) +
                                     agram.right (agram.length() - i - 1));
                if (ait != alphaset.end())
                    return true;
            }
            return false;
        }


        default: return false;
    }
}

//---------------------------------------------------------------------------
//  numAnagrams
//
//! Determine the number of valid anagrams of a word.
//
//! @param word the word
//! @return the number of valid anagrams
//---------------------------------------------------------------------------
int
WordEngine::numAnagrams (const QString& word) const
{
    QString alpha = Auxil::getAlphagram (word);
    return numAnagramsMap.contains (alpha) ? numAnagramsMap[alpha] : 0;
}

//---------------------------------------------------------------------------
//  nonGraphSearch
//
//! Search for valid words matching conditions that can be matched without
//! searching the word graph.
//
//! @param spec the search specification
//
//! @return a list of acceptable words matching the In Word List conditions
//---------------------------------------------------------------------------
QStringList
WordEngine::nonGraphSearch (const SearchSpec& spec) const
{
    QStringList wordList;
    set<QString> finalWordSet;
    set<QString>::iterator sit;
    int conditionNum = 0;

    const int MAX_ANAGRAMS = 65535;
    int minAnagrams = 0;
    int maxAnagrams = MAX_ANAGRAMS;

    // Look for InWordList conditions first, to narrow the search as much as
    // possible
    QListIterator<SearchCondition> it (spec.conditions);
    while (it.hasNext()) {
        const SearchCondition& condition = it.next();

        // Note the minimum and maximum number of anagrams from any Number of
        // Anagrams conditions
        if (condition.type == SearchCondition::NumAnagrams) {
            if ((condition.minValue > maxAnagrams) ||
                (condition.maxValue < minAnagrams))
                return wordList;
            minAnagrams = condition.minValue;
            maxAnagrams = condition.maxValue;
        }

        // Only InWordList conditions allowed beyond this point - look up
        // words for acceptability and combine the word lists
        if (condition.type != SearchCondition::InWordList)
            continue;
        QStringList words = condition.stringValue.split (QChar (' '));

        set<QString> wordSet;
        QStringList::iterator wit;
        for (wit = words.begin(); wit != words.end(); ++wit) {
            if (isAcceptable (*wit))
                wordSet.insert (*wit);
        }

        // Combine search result set with words already found
        if (!conditionNum) {
            finalWordSet = wordSet;
        }

        else if (spec.conjunction) {
            set<QString> conjunctionSet;
            for (sit = wordSet.begin(); sit != wordSet.end(); ++sit) {
                if (finalWordSet.find (*sit) != finalWordSet.end())
                    conjunctionSet.insert (*sit);
            }
            if (conjunctionSet.empty())
                return wordList;
            finalWordSet = conjunctionSet;
        }

        else {
            for (sit = wordSet.begin(); sit != wordSet.end(); ++sit) {
                finalWordSet.insert (*sit);
            }
        }

        ++conditionNum;
    }

    // Now limit the set only to those words containing the requisite number
    // of anagrams.  If some words are already in the finalWordSet, then only
    // test those words.  Otherwise, run through the map of number of anagrams
    // and pull out all words matching the conditions.
    if (!finalWordSet.empty() &&
        ((minAnagrams > 0) || (maxAnagrams < MAX_ANAGRAMS)))
    {
        set<QString> wordSet;
        for (sit = finalWordSet.begin(); sit != finalWordSet.end();
                ++sit)
        {
            int numAnagrams = numAnagramsMap[ Auxil::getAlphagram (*sit) ];
            if ((numAnagrams >= minAnagrams) &&
                (numAnagrams <= maxAnagrams))
            {
                wordSet.insert (*sit);
            }
        }
        finalWordSet = wordSet;
    }

    // Transform word set into word list and return it
    for (sit = finalWordSet.begin(); sit != finalWordSet.end(); ++sit) {
        wordList << *sit;
    }

    return wordList;
}

//---------------------------------------------------------------------------
//  addDefinition
//
//! Add a word with its definition.  Parse the definition and separate its
//! parts of speech.
//
//! @param word the word
//! @param definition the definition
//---------------------------------------------------------------------------
void
WordEngine::addDefinition (const QString& word, const QString& definition)
{
    if (word.isEmpty() || definition.isEmpty())
        return;

    QRegExp posRegex (QString ("\\[(\\w+)"));
    multimap<QString, QString> defMap;
    QStringList defs = definition.split (" / ");
    QString def;
    foreach (def, defs) {
        QString pos;
        if (posRegex.indexIn (def, 0) >= 0) {
            pos = posRegex.cap (1);
        }
        defMap.insert (make_pair (pos, def));
    }
    definitions.insert (make_pair (word, defMap));
}

//---------------------------------------------------------------------------
//  replaceDefinitionLinks
//
//! Replace links in a definition with the definitions of the words they are
//! linked to.  A string is assumed to have a maximum of one link.  Links may
//! be followed recursively to the maximum depth specified.
//
//! @param definition the definition with links to be replaced
//! @param maxDepth the maximum number of recursive links to replace
//! @param useFollow true if the "follow" replacement should be used
//
//! @return a string with links replaced
//---------------------------------------------------------------------------
QString
WordEngine::replaceDefinitionLinks (const QString& definition, int maxDepth,
                                    bool useFollow) const
{
    QRegExp followRegex (QString ("\\{(\\w+)=(\\w+)\\}"));
    QRegExp replaceRegex (QString ("\\<(\\w+)=(\\w+)\\>"));

    // Try to match the follow regex and the replace regex.  If a follow regex
    // is ever matched, then the "follow" replacements should always be used,
    // even if the "replace" regex is matched in a later iteration.
    QRegExp* matchedRegex = 0;
    int index = followRegex.indexIn (definition, 0);
    if (index >= 0) {
        matchedRegex = &followRegex;
        useFollow = true;
    }
    else {
        index = replaceRegex.indexIn (definition, 0);
        matchedRegex = &replaceRegex;
    }

    if (index < 0)
        return definition;

    QString modified (definition);
    QString word = matchedRegex->cap (1);
    QString pos = matchedRegex->cap (2);

    QString replacement;
    if (!maxDepth) {
        replacement = word;
    }
    else {
        QString upper = word.toUpper();
        if (useFollow) {
            replacement = (matchedRegex == &followRegex) ?
                word + " (" + getSubDefinition (upper, pos) + ")" :
                getSubDefinition (upper, pos);
        }
        else {
            replacement = upper + ", " + getSubDefinition (upper, pos);
        }
    }

    modified.replace (index, matchedRegex->matchedLength(), replacement);
    return maxDepth ? replaceDefinitionLinks (modified, maxDepth - 1,
                                              useFollow) : modified;
}

//---------------------------------------------------------------------------
//  getSubDefinition
//
//! Return the definition associated with a word and a part of speech.  If
//! more than one definition is given for a part of speech, pick the first
//! one.
//
//! @param word the word
//! @param pos the part of speech
//
//! @return the definition substring
//---------------------------------------------------------------------------
QString
WordEngine::getSubDefinition (const QString& word, const QString& pos) const
{
    std::map<QString, std::multimap<QString, QString> >::const_iterator it =
        definitions.find (word);
    if (it == definitions.end())
        return QString::null;

    const multimap<QString, QString>& mmap = it->second;
    multimap<QString, QString>::const_iterator mit = mmap.find (pos);
    if (mit == mmap.end())
        return QString::null;

    return mit->second.left (mit->second.indexOf (" ["));
}
