/*
*******************************************************************************
*   Copyright (C) 1997-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   Date        Name        Description
*   06/21/00    aliu        Creation.
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/utrans.h"
#include "unicode/putil.h"
#include "unicode/rep.h"
#include "unicode/translit.h"
#include "unicode/unifilt.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "cpputils.h"
#include "rbt.h"

// Following macro is to be followed by <return value>';' or just ';'
#define utrans_ENTRY(s) if ((s)==NULL || U_FAILURE(*(s))) return

/********************************************************************
 * Replaceable-UReplaceableCallbacks glue
 ********************************************************************/

/**
 * Make a UReplaceable + UReplaceableCallbacks into a Replaceable object.
 */
U_NAMESPACE_BEGIN
class ReplaceableGlue : public Replaceable {

    UReplaceable *rep;
    UReplaceableCallbacks *func;

    /**
     * The address of this static class variable serves as this class's ID
     * for ICU "poor man's RTTI".
     */
    static const char fgClassID;

public:

    ReplaceableGlue(UReplaceable *replaceable,
                    UReplaceableCallbacks *funcCallback);

    virtual ~ReplaceableGlue();

    virtual void handleReplaceBetween(int32_t start,
                                      int32_t limit,
                                      const UnicodeString& text);

    virtual void extractBetween(int32_t start,
                                int32_t limit,
                                UnicodeString& target) const;

    virtual void copy(int32_t start, int32_t limit, int32_t dest);

    // virtual Replaceable *clone() const { return NULL; } same as default

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @draft ICU 2.2
     */
    virtual inline UClassID getDynamicClassID() const { return getStaticClassID(); }

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @draft ICU 2.2
     */
    static inline UClassID getStaticClassID() { return (UClassID)&fgClassID; }

protected:

    virtual int32_t getLength() const;

    virtual UChar getCharAt(int32_t offset) const;

    virtual UChar32 getChar32At(int32_t offset) const;
};


const char ReplaceableGlue::fgClassID=0;

ReplaceableGlue::ReplaceableGlue(UReplaceable *replaceable,
                                 UReplaceableCallbacks *funcCallback)
  : Replaceable()
{
    this->rep = replaceable;
    this->func = funcCallback;
}

ReplaceableGlue::~ReplaceableGlue() {}

int32_t ReplaceableGlue::getLength() const {
    return (*func->length)(rep);
}

UChar ReplaceableGlue::getCharAt(int32_t offset) const {
    return (*func->charAt)(rep, offset);
}

UChar32 ReplaceableGlue::getChar32At(int32_t offset) const {
    return (*func->char32At)(rep, offset);
}

void ReplaceableGlue::handleReplaceBetween(int32_t start,
                          int32_t limit,
                          const UnicodeString& text) {
    (*func->replace)(rep, start, limit, text.getBuffer(), text.length());
}

void ReplaceableGlue::extractBetween(int32_t start,
                                     int32_t limit,
                                     UnicodeString& target) const {
    (*func->extract)(rep, start, limit, target.getBuffer(limit-start));
    target.releaseBuffer(limit-start);
}

void ReplaceableGlue::copy(int32_t start, int32_t limit, int32_t dest) {
    (*func->copy)(rep, start, limit, dest);
}
U_NAMESPACE_END
/********************************************************************
 * General API
 ********************************************************************/
U_NAMESPACE_USE
#if 0

U_CAPI UTransliterator*
utrans_open(const char* id,
            UTransDirection dir,
            UParseError* parseError,
            UErrorCode* status) {

    utrans_ENTRY(status) NULL;

    if (id == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    UnicodeString ID(id, ""); // use invariant converter
    Transliterator *trans = NULL;

    trans = Transliterator::createInstance(ID, dir, *parseError, *status);

    if (trans == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return (UTransliterator*) trans;
}

U_CAPI UTransliterator*
utrans_openRules(const char* id,
                 const UChar* rules,
                 int32_t rulesLength, /* -1 if null-terminated */
                 UTransDirection dir,
                 UParseError* parseErr, /* may be NULL */
                 UErrorCode* status) {

    utrans_ENTRY(status) NULL;

    if (id == NULL || rules == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    UnicodeString ID(id, ""); // use invariant converter
    UnicodeString ruleStr(rulesLength < 0,
                          rules,
                          rulesLength); // r-o alias

    RuleBasedTransliterator *trans = NULL;

    // Use if() to avoid construction of ParseError object on stack
    // unless it is called for by user.
    if (parseErr != NULL) {
        trans = new RuleBasedTransliterator(ID, ruleStr, dir,
                                            NULL, *parseErr, *status);
    } else {
        trans = new RuleBasedTransliterator(ID, ruleStr, dir,
                                            NULL, *status);
    }

    if (trans == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    } else if (U_FAILURE(*status)) {
        delete trans;
        trans = NULL;
    }
    return (UTransliterator*) trans;
}
#endif

U_CAPI UTransliterator* U_EXPORT2
utrans_open(const char* id,
            UTransDirection dir,
            const UChar* rules,         /* may be Null */
            int32_t rulesLength,        /* -1 if null-terminated */ 
            UParseError* parseError,    /* may be Null */
            UErrorCode* status) {

    utrans_ENTRY(status) NULL;

    if (id == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    UParseError temp;
    
    if(parseError == NULL){
        parseError = &temp;
    }
    
    UnicodeString ID(id, ""); // use invariant converter

    if(rules==NULL){

        Transliterator *trans = NULL;

        trans = Transliterator::createInstance(ID, dir, *parseError, *status);
        
        if(U_FAILURE(*status)){
            return NULL;
        }
        return (UTransliterator*) trans;
    }else{
        UnicodeString ruleStr(rulesLength < 0,
                              rules,
                              rulesLength); // r-o alias

        RuleBasedTransliterator *trans = NULL;
        trans = new RuleBasedTransliterator(ID, ruleStr, dir,
                                            NULL, *parseError, *status);
        if (trans == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        } else if (U_FAILURE(*status)) {
            delete trans;
            trans = NULL;
        }
        return (UTransliterator*) trans;
    }
}

U_CAPI UTransliterator* U_EXPORT2
utrans_openInverse(const UTransliterator* trans,
                   UErrorCode* status) {

    utrans_ENTRY(status) NULL;

    UTransliterator* result =
        (UTransliterator*) ((Transliterator*) trans)->createInverse(*status);

    return result;
}

U_CAPI UTransliterator* U_EXPORT2
utrans_clone(const UTransliterator* trans,
             UErrorCode* status) {

    utrans_ENTRY(status) NULL;

    if (trans == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    Transliterator *t = ((Transliterator*) trans)->clone();
    if (t == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    }
    return (UTransliterator*) t;
}

U_CAPI void U_EXPORT2
utrans_close(UTransliterator* trans) {
    delete (Transliterator*) trans;
}

U_CAPI int32_t U_EXPORT2
utrans_getID(const UTransliterator* trans,
             char* buf,
             int32_t bufCapacity) {
    return ((Transliterator*) trans)->getID().extract(0, 0x7fffffff, buf, bufCapacity, "");
}

U_CAPI void U_EXPORT2
utrans_register(UTransliterator* adoptedTrans,
                UErrorCode* status) {
    utrans_ENTRY(status);
    // status currently ignored; may remove later
    Transliterator::registerInstance((Transliterator*) adoptedTrans);
}

U_CAPI void U_EXPORT2
utrans_unregister(const char* id) {
    UnicodeString ID(id, ""); // use invariant converter
    Transliterator::unregister(ID);
}

U_CAPI void U_EXPORT2
utrans_setFilter(UTransliterator* trans,
                 const UChar* filterPattern,
                 int32_t filterPatternLen,
                 UErrorCode* status) {

    utrans_ENTRY(status);
    UnicodeFilter* filter = NULL;
    if (filterPattern != NULL && *filterPattern != 0) {
        // Create read only alias of filterPattern:
        UnicodeString pat(filterPatternLen < 0, filterPattern, filterPatternLen);
        filter = new UnicodeSet(pat, *status);
        /* test for NULL */
        if (filter == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if (U_FAILURE(*status)) {
            delete filter;
            filter = NULL;
        }
    }
    ((Transliterator*) trans)->adoptFilter(filter);
}

U_CAPI int32_t U_EXPORT2
utrans_countAvailableIDs(void) {
    return Transliterator::countAvailableIDs();
}

U_CAPI int32_t U_EXPORT2
utrans_getAvailableID(int32_t index,
                      char* buf, // may be NULL
                      int32_t bufCapacity) {
    return Transliterator::getAvailableID(index).extract(0, 0x7fffffff, buf, bufCapacity, "");
}

/********************************************************************
 * Transliteration API
 ********************************************************************/

U_CAPI void U_EXPORT2
utrans_trans(const UTransliterator* trans,
             UReplaceable* rep,
             UReplaceableCallbacks* repFunc,
             int32_t start,
             int32_t* limit,
             UErrorCode* status) {

    utrans_ENTRY(status);

    if (trans == 0 || rep == 0 || repFunc == 0 || limit == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    ReplaceableGlue r(rep, repFunc);

    *limit = ((Transliterator*) trans)->transliterate(r, start, *limit);
}

U_CAPI void U_EXPORT2
utrans_transIncremental(const UTransliterator* trans,
                        UReplaceable* rep,
                        UReplaceableCallbacks* repFunc,
                        UTransPosition* pos,
                        UErrorCode* status) {

    utrans_ENTRY(status);

    if (trans == 0 || rep == 0 || repFunc == 0 || pos == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    ReplaceableGlue r(rep, repFunc);

    ((Transliterator*) trans)->transliterate(r, *pos, *status);
}

U_CAPI void U_EXPORT2
utrans_transUChars(const UTransliterator* trans,
                   UChar* text,
                   int32_t* textLength,
                   int32_t textCapacity,
                   int32_t start,
                   int32_t* limit,
                   UErrorCode* status) {

    utrans_ENTRY(status);

    if (trans == 0 || text == 0 || limit == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
 
    int32_t textLen = (textLength == NULL || *textLength < 0)
        ? u_strlen(text) : *textLength;
    // writeable alias: for this ct, len CANNOT be -1 (why?)
    UnicodeString str(text, textLen, textCapacity);

    *limit = ((Transliterator*) trans)->transliterate(str, start, *limit);

    // Copy the string buffer back to text (only if necessary)
    // and fill in *neededCapacity (if neededCapacity != NULL).
    textLen = str.extract(text, textCapacity, *status);
    if(textLength != NULL) {
        *textLength = textLen;
    }
}

U_CAPI void U_EXPORT2
utrans_transIncrementalUChars(const UTransliterator* trans,
                              UChar* text,
                              int32_t* textLength,
                              int32_t textCapacity,
                              UTransPosition* pos,
                              UErrorCode* status) {

    utrans_ENTRY(status);

    if (trans == 0 || text == 0 || pos == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    int32_t textLen = (textLength == NULL || *textLength < 0)
        ? u_strlen(text) : *textLength;
    // writeable alias: for this ct, len CANNOT be -1 (why?)
    UnicodeString str(text, textLen, textCapacity);

    ((Transliterator*) trans)->transliterate(str, *pos, *status);

    // Copy the string buffer back to text (only if necessary)
    // and fill in *neededCapacity (if neededCapacity != NULL).
    textLen = str.extract(text, textCapacity, *status);
    if(textLength != NULL) {
        *textLength = textLen;
    }
}

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
